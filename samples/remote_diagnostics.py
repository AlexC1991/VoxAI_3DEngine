import socket
import json
import base64
import time

def send_cmd(cmd, id=1):
    packet = {
        "id": id,
        "key": "VOXAI_MASTER_BRAIN",
        "cmd": cmd["cmd"],
    }
    packet.update(cmd)
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 5555))
    s.sendall((json.dumps(packet) + "\n").encode("utf-8"))
    
    # Read until newline
    data = b""
    while not data.endswith(b"\n"):
        chunk = s.recv(4096)
        if not chunk: break
        data += chunk
    
    s.close()
    return json.loads(data.decode("utf-8"))

print("--- VoxAI v0.2 Remote Control Test ---")

# 1. PING
print("\n[1] PING")
print(send_cmd({"cmd": "PING"}, 1))

# 2. SPAWN_NODE
print("\n[2] SPAWN_NODE (MeshInstance3D)")
spawn_res = send_cmd({
    "cmd": "SPAWN_NODE",
    "type": "MeshInstance3D",
    "name": "TestCube",
    "parent": "/root"
}, 2)
print(spawn_res)

target_path = spawn_res.get("data")

# 3. GET_TREE
print("\n[3] GET_TREE")
print(send_cmd({"cmd": "GET_TREE"}, 3))

# 4. INJECT_SCRIPT into /root/TestCube
if target_path:
    print(f"\n[4] INJECT_SCRIPT into {target_path}")
    script_str = """extends MeshInstance3D
func brain_check():
    return 'Brain Active'
"""
    b64_script = base64.b64encode(script_str.encode("utf-8")).decode("utf-8")
    
    inject_res = send_cmd({
        "id": 4,
        "cmd": "INJECT_SCRIPT",
        "target": target_path,
        "code": b64_script
    })
    print(inject_res)

# 5. CALL_METHOD (verify script works)
if target_path:
    print("\n[5] CALL_METHOD (brain_check)")
    print(send_cmd({
        "id": 5,
        "cmd": "CALL_METHOD",
        "target": target_path,
        "method": "brain_check",
        "args": []
    }))

# 6. QUIT
print("\n[6] QUIT")
print(send_cmd({"cmd": "QUIT"}, 6))
