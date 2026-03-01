/**************************************************************************/
/*  voxai_brain.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             VOXAI ENGINE                               */
/**************************************************************************/

#pragma once

#include "core/io/tcp_server.h"
#include "core/io/json.h"
#include "core/object/object.h"
#include "core/templates/list.h"
#include "core/os/mutex.h"

class VoxAIBrain : public Object {
	GDCLASS(VoxAIBrain, Object);

	static VoxAIBrain *singleton;

	struct Command {
		int id;
		Ref<StreamPeerTCP> conn;
		Dictionary data;
	};

	Mutex queue_mutex;
	List<Command> command_queue;

	Ref<TCPServer> server;
	List<Ref<StreamPeerTCP>> connections;
	
	String master_key = "VOXAI_MASTER_BRAIN";

	void _handle_command(const Command &p_cmd);
	void _send_response(Ref<StreamPeerTCP> p_conn, int p_id, const String &p_cmd, const String &p_status, int p_code, const Variant &p_data = Variant(), const String &p_error = "");
	void _push_event(const String &p_event_class, const Dictionary &p_data);

	// Command Handlers
	void _cmd_ping(const Command &p_cmd);
	void _cmd_load_scene(const Command &p_cmd);
	void _cmd_spawn_node(const Command &p_cmd);
	void _cmd_free_node(const Command &p_cmd);
	void _cmd_inject_script(const Command &p_cmd);
	void _cmd_call_method(const Command &p_cmd);
	void _cmd_set_props(const Command &p_cmd);
	void _cmd_get_state(const Command &p_cmd);
	void _cmd_get_tree(const Command &p_cmd);

	Dictionary _serialize_node(class Node *p_node);

public:
	static VoxAIBrain *get_singleton() { return singleton; }

	void start(uint16_t p_port = 5555);
	void iteration();
	void stop();

	VoxAIBrain();
	~VoxAIBrain();
};
