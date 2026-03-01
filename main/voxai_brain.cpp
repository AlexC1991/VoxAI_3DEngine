/**************************************************************************/
/*  voxai_brain.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             VOXAI ENGINE                               */
/**************************************************************************/

#include "voxai_brain.h"
#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/main/node.h"
#include "modules/gdscript/gdscript.h"

VoxAIBrain *VoxAIBrain::singleton = nullptr;

VoxAIBrain::VoxAIBrain() {
	singleton = this;
	server.instantiate();
}

VoxAIBrain::~VoxAIBrain() {
	stop();
	singleton = nullptr;
}

void VoxAIBrain::start(uint16_t p_port) {
	Error err = server->listen(p_port);
	if (err != OK) {
		ERR_PRINT("VoxAIBrain: Failed to start TCP server on port " + itos(p_port));
		return;
	}
	print_line("VoxAIBrain: Listening on port " + itos(p_port));
}

void VoxAIBrain::stop() {
	server->stop();
	connections.clear();
}

void VoxAIBrain::iteration() {
	// 1. Process Command Queue (MAIN THREAD)
	{
		MutexLock lock(queue_mutex);
		int processed = 0;
		while (command_queue.size() > 0 && processed < 50) {
			Command cmd = command_queue.front()->get();
			command_queue.pop_front();
			
			_handle_command(cmd);
			
			processed++;
		}
	}

	if (!server->is_listening()) {
		return;
	}

	// 2. Accept new connections (NETWORKING)
	if (server->is_connection_available()) {
		Ref<StreamPeerTCP> conn = server->take_connection();
		connections.push_back(conn);
		print_line("VoxAIBrain: New IDE connection established.");
	}

	// 3. Process existing connections (NETWORKING)
	for (List<Ref<StreamPeerTCP>>::Element *E = connections.front(); E; ) {
		Ref<StreamPeerTCP> conn = E->get();
		List<Ref<StreamPeerTCP>>::Element *Next = E->next();

		if (conn->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
			connections.erase(E);
			print_line("VoxAIBrain: IDE connection lost.");
			E = Next;
			continue;
		}

		int available = conn->get_available_bytes();
		if (available > 0) {
			Vector<uint8_t> buffer;
			buffer.resize(available);
			conn->get_data(buffer.ptrw(), available);
			
			String json_str = String::utf8((const char *)buffer.ptr(), available);
			
			JSON json;
			Error err = json.parse(json_str);
			Variant data = json.get_data();

			if (err == OK) {
				Dictionary d = data;
				if (!d.has("key") || String(d["key"]) != master_key) {
					_send_response(conn, d.get("id", 0), d.get("cmd", "unknown"), "ERROR", 401, Variant(), "Unauthorized");
				} else {
					Command cmd;
					cmd.id = d.get("id", 0);
					cmd.conn = conn;
					cmd.data = d;
					
					MutexLock lock(queue_mutex);
					command_queue.push_back(cmd);
				}
			} else {
				_send_response(conn, 0, "unknown", "ERROR", 400, Variant(), "Invalid JSON: " + json.get_error_message());
			}
		}

		E = Next;
	}
}

void VoxAIBrain::_handle_command(const Command &p_cmd) {
	String cmd_name = p_cmd.data.get("cmd", "");

	if (cmd_name == "PING") {
		_cmd_ping(p_cmd);
	} else if (cmd_name == "LOAD_SCENE") {
		_cmd_load_scene(p_cmd);
	} else if (cmd_name == "SPAWN_NODE") {
		_cmd_spawn_node(p_cmd);
	} else if (cmd_name == "FREE_NODE") {
		_cmd_free_node(p_cmd);
	} else if (cmd_name == "INJECT_SCRIPT") {
		_cmd_inject_script(p_cmd);
	} else if (cmd_name == "CALL_METHOD") {
		_cmd_call_method(p_cmd);
	} else if (cmd_name == "SET_PROPS") {
		_cmd_set_props(p_cmd);
	} else if (cmd_name == "GET_STATE") {
		_cmd_get_state(p_cmd);
	} else if (cmd_name == "GET_TREE") {
		_cmd_get_tree(p_cmd);
	} else if (cmd_name == "QUIT") {
		_send_response(p_cmd.conn, p_cmd.id, cmd_name, "OK", 200, "Shutting down");
		SceneTree::get_singleton()->quit();
	} else {
		_send_response(p_cmd.conn, p_cmd.id, cmd_name, "ERROR", 404, Variant(), "Unknown command: " + cmd_name);
	}
}

void VoxAIBrain::_send_response(Ref<StreamPeerTCP> p_conn, int p_id, const String &p_cmd, const String &p_status, int p_code, const Variant &p_data, const String &p_error) {
	Dictionary res;
	res["id"] = p_id;
	res["cmd"] = p_cmd;
	res["status"] = p_status;
	res["code"] = p_code;
	res["data"] = p_data;
	res["error"] = p_error.is_empty() ? Variant() : Variant(p_error);

	String json = JSON::stringify(res) + "\n";
	CharString cs = json.utf8();
	p_conn->put_data((const uint8_t *)cs.ptr(), cs.length());
}

void VoxAIBrain::_cmd_ping(const Command &p_cmd) {
	Dictionary data;
	data["engine"] = "VoxAI_3D";
	data["version"] = "0.2";
	_send_response(p_cmd.conn, p_cmd.id, "PING", "OK", 200, data);
}

void VoxAIBrain::_cmd_load_scene(const Command &p_cmd) {
	String path = p_cmd.data.get("path", "");
	if (path.is_empty()) {
		_send_response(p_cmd.conn, p_cmd.id, "LOAD_SCENE", "ERROR", 400, Variant(), "Missing 'path'");
		return;
	}

	SceneTree::get_singleton()->change_scene_to_file(path);
	_send_response(p_cmd.conn, p_cmd.id, "LOAD_SCENE", "OK", 200, "Scene bridge requested");
}

void VoxAIBrain::_cmd_spawn_node(const Command &p_cmd) {
	String type = p_cmd.data.get("type", "");
	String name = p_cmd.data.get("name", "");
	String parent_path = p_cmd.data.get("parent", "/root");

	if (type.is_empty()) {
		_send_response(p_cmd.conn, p_cmd.id, "SPAWN_NODE", "ERROR", 400, Variant(), "Missing 'type'");
		return;
	}

	Object *obj = ClassDB::instantiate(type);
	if (!obj) {
		_send_response(p_cmd.conn, p_cmd.id, "SPAWN_NODE", "ERROR", 422, Variant(), "Failed to instantiate type: " + type);
		return;
	}

	Node *node = Object::cast_to<Node>(obj);
	if (!node) {
		memdelete(obj);
		_send_response(p_cmd.conn, p_cmd.id, "SPAWN_NODE", "ERROR", 422, Variant(), "Type is not a Node: " + type);
		return;
	}

	if (!name.is_empty()) {
		node->set_name(name);
	}

	Node *parent = SceneTree::get_singleton()->get_root()->get_node(parent_path);
	if (!parent) {
		memdelete(node);
		_send_response(p_cmd.conn, p_cmd.id, "SPAWN_NODE", "ERROR", 404, Variant(), "Parent not found: " + parent_path);
		return;
	}

	parent->add_child(node);
	_send_response(p_cmd.conn, p_cmd.id, "SPAWN_NODE", "OK", 200, node->get_path());
}

void VoxAIBrain::_cmd_free_node(const Command &p_cmd) {
	String target = p_cmd.data.get("target", "");
	Node *node = SceneTree::get_singleton()->get_root()->get_node(target);
	
	if (!node) {
		_send_response(p_cmd.conn, p_cmd.id, "FREE_NODE", "ERROR", 404, Variant(), "Target not found: " + target);
		return;
	}

	node->queue_free();
	_send_response(p_cmd.conn, p_cmd.id, "FREE_NODE", "OK", 200);
}

void VoxAIBrain::_cmd_inject_script(const Command &p_cmd) {
	String target = p_cmd.data.get("target", "");
	String b64_code = p_cmd.data.get("code", "");

	Node *node = SceneTree::get_singleton()->get_root()->get_node(target);
	if (!node) {
		_send_response(p_cmd.conn, p_cmd.id, "INJECT_SCRIPT", "ERROR", 404, Variant(), "Target not found: " + target);
		return;
	}

	String code = CoreBind::Marshalls::get_singleton()->base64_to_utf8(b64_code);
	
	Ref<GDScript> script;
	script.instantiate();
	script->set_source_code(code);
	script->set_path("res://injected_" + node->get_name() + ".gd");
	Error err = script->reload();

	if (err != OK) {
		_send_response(p_cmd.conn, p_cmd.id, "INJECT_SCRIPT", "ERROR", 500, Variant(), "Script compilation failed");
		return;
	}

	node->set_script(script);
	if (node->has_method("_ready")) {
		node->call("_ready");
	}
	OS::get_singleton()->print("VoxAIBrain: Injected and activated script in %s\n", String(node->get_name()).utf8().get_data());
	_send_response(p_cmd.conn, p_cmd.id, "INJECT_SCRIPT", "OK", 200);
}

void VoxAIBrain::_cmd_call_method(const Command &p_cmd) {
	String target = p_cmd.data.get("target", "");
	String method = p_cmd.data.get("method", "");
	Array args = p_cmd.data.get("args", Array());

	Node *node = SceneTree::get_singleton()->get_root()->get_node(target);
	if (!node) {
		_send_response(p_cmd.conn, p_cmd.id, "CALL_METHOD", "ERROR", 404, Variant(), "Target not found: " + target);
		return;
	}

	Variant ret = node->callv(method, args);
	_send_response(p_cmd.conn, p_cmd.id, "CALL_METHOD", "OK", 200, ret);
}

void VoxAIBrain::_cmd_set_props(const Command &p_cmd) {
	String target = p_cmd.data.get("target", "");
	Dictionary props = p_cmd.data.get("props", Dictionary());

	Node *node = SceneTree::get_singleton()->get_root()->get_node(target);
	if (!node) {
		_send_response(p_cmd.conn, p_cmd.id, "SET_PROPS", "ERROR", 404, Variant(), "Target not found: " + target);
		return;
	}

	Array keys = props.keys();
	for (int i = 0; i < keys.size(); i++) {
		Variant K = keys[i];
		Variant V = props[K];

		Variant existing = node->get(K);
		if (V.get_type() == Variant::ARRAY) {
			Array arr = V;
			if (existing.get_type() == Variant::VECTOR3 && arr.size() == 3) {
				V = Vector3((float)arr[0], (float)arr[1], (float)arr[2]);
			} else if (existing.get_type() == Variant::VECTOR2 && arr.size() == 2) {
				V = Vector2((float)arr[0], (float)arr[1]);
			}
		}

		OS::get_singleton()->print("VoxAIBrain: Setting property '%s' on node '%s' to %s\n", String(K).utf8().get_data(), String(node->get_name()).utf8().get_data(), String(V).utf8().get_data());
		node->set(K, V);
	}

	_send_response(p_cmd.conn, p_cmd.id, "SET_PROPS", "OK", 200);
}

void VoxAIBrain::_cmd_get_state(const Command &p_cmd) {
	String target = p_cmd.data.get("target", "");
	Array props = p_cmd.data.get("props", Array());

	Node *node = SceneTree::get_singleton()->get_root()->get_node(target);
	if (!node) {
		_send_response(p_cmd.conn, p_cmd.id, "GET_STATE", "ERROR", 404, Variant(), "Target not found: " + target);
		return;
	}

	Dictionary res;
	for (int i = 0; i < props.size(); i++) {
		String p = props[i];
		Variant v = node->get(p);
		
		if (v.get_type() == Variant::VECTOR3) {
			Vector3 vec = v;
			Array arr;
			arr.push_back(vec.x);
			arr.push_back(vec.y);
			arr.push_back(vec.z);
			res[p] = arr;
		} else if (v.get_type() == Variant::VECTOR2) {
			Vector2 vec = v;
			Array arr;
			arr.push_back(vec.x);
			arr.push_back(vec.y);
			res[p] = arr;
		} else {
			res[p] = v;
		}
	}

	_send_response(p_cmd.conn, p_cmd.id, "GET_STATE", "OK", 200, res);
}

void VoxAIBrain::_cmd_get_tree(const Command &p_cmd) {
	Node *root = SceneTree::get_singleton()->get_root();
	_send_response(p_cmd.conn, p_cmd.id, "GET_TREE", "OK", 200, _serialize_node(root));
}

Dictionary VoxAIBrain::_serialize_node(Node *p_node) {
	Dictionary d;
	d["name"] = p_node->get_name();
	d["type"] = p_node->get_class();
	d["path"] = p_node->get_path();
	d["has_script"] = !p_node->get_script().is_null();

	Array children;
	for (int i = 0; i < p_node->get_child_count(); i++) {
		children.push_back(_serialize_node(p_node->get_child(i)));
	}
	d["children"] = children;

	return d;
}
