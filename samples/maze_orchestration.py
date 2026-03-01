import socket
import json
import base64
import time
import sys

# --- Configuration ---
HOST = "127.0.0.1"
PORT = 5555
KEY = "VOXAI_MASTER_BRAIN"

def list_to_vec3(v):
    if isinstance(v, list) and len(v) == 3:
        return f"({v[0]}, {v[1]}, {v[2]})"
    return str(v)

def send_cmd(cmd, id=1, quiet=False):
    packet = {"id": id, "key": KEY}
    packet.update(cmd)
    
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2.0)
        s.connect((HOST, PORT))
        s.sendall((json.dumps(packet) + "\n").encode("utf-8"))
        
        data = b""
        while not data.endswith(b"\n"):
            chunk = s.recv(4096)
            if not chunk: break
            data += chunk
        s.close()
        
        res = json.loads(data.decode("utf-8"))
        if not quiet:
            status = res.get("status", "UNKNOWN")
            code = res.get("code", 0)
            if code != 200:
                print(f"  [!] Cmd {cmd['cmd']} failed: {res.get('error', 'Unknown error')}")
            else:
                pass # Success
        return res
    except ConnectionRefusedError:
        print(f"  [ERROR] Connection Refused! Is VoxAI_3DEngine.exe running on port {PORT}?")
        sys.exit(1)
    except Exception as e:
        print(f"  [ERROR] Socket error: {e}")
        return {"status": "ERROR", "error": str(e)}

print("\n" + "="*50)
print("   🌌 VoxAI Agentic Orchestrator - Maze POC")
print("="*50)

# 1. Reset Scene
print("\n[1] Resetting Scene Hierarchy...")
send_cmd({"cmd": "FREE_NODE", "target": "/root/Main/VisualScene"}, 0, quiet=True)
res = send_cmd({"cmd": "SPAWN_NODE", "type": "Node3D", "name": "VisualScene", "parent": "/root/Main"}, 1)
if res.get("status") == "OK":
    print("  -> Scene Container Ready.")

# 2. Setup Environment & Camera
print("\n[2] Configuring Lighting & Camera...")
send_cmd({"cmd": "SPAWN_NODE", "type": "WorldEnvironment", "name": "Env", "parent": "/root/Main/VisualScene"}, 2)
env_init = """extends WorldEnvironment
func _ready():
    environment = Environment.new()
    environment.background_mode = Environment.BG_SKY
    environment.sky = Sky.new()
    environment.sky.sky_material = ProceduralSkyMaterial.new()
    environment.tonemap_mode = Environment.TONE_MAP_FILMIC
    environment.glow_enabled = true
    environment.glow_bloom = 0.5
    environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
    environment.ambient_light_sky_contribution = 0.5
"""
send_cmd({"cmd": "INJECT_SCRIPT", "target": "/root/Main/VisualScene/Env", "code": base64.b64encode(env_init.encode()).decode()}, 3)

send_cmd({"cmd": "SPAWN_NODE", "type": "Camera3D", "name": "MainCamera", "parent": "/root/Main/VisualScene"}, 4)
send_cmd({"cmd": "SET_PROPS", "target": "/root/Main/VisualScene/MainCamera", "props": {
    "position": [0, 25, 25],
    "rotation_degrees": [-45, 0, 0],
    "current": True
}}, 5)

send_cmd({"cmd": "SPAWN_NODE", "type": "DirectionalLight3D", "name": "Sun", "parent": "/root/Main/VisualScene"}, 6)
send_cmd({"cmd": "SET_PROPS", "target": "/root/Main/VisualScene/Sun", "props": {"rotation_degrees": [-60, 45, 0], "light_energy": 2.5, "shadow_enabled": True}}, 7)

# 3. Create Floor
print("\n[3] Spawning Neon Grid Floor...")
send_cmd({"cmd": "SPAWN_NODE", "type": "MeshInstance3D", "name": "Floor", "parent": "/root/Main/VisualScene"}, 8)
floor_init = """extends MeshInstance3D
func _ready():
    mesh = PlaneMesh.new()
    mesh.size = Vector2(40, 40)
    var mat = StandardMaterial3D.new()
    mat.albedo_color = Color(0, 1, 0)
    mat.emission_enabled = true
    mat.emission = Color(0, 1, 0)
    mat.emission_energy_multiplier = 0.1
    mesh.material = mat
"""
send_cmd({"cmd": "INJECT_SCRIPT", "target": "/root/Main/VisualScene/Floor", "code": base64.b64encode(floor_init.encode()).decode()}, 9)

# 4. Construct Maze Walls
print("\n[4] Constructing Maze Geometry (Processing 36 units)...")
maze_map = [
    "##########",
    "#........#",
    "#.######.#",
    "#.#....#.#",
    "#.#.##.#.#",
    "#.#..#.#.#",
    "#.#.##.#.#",
    "#.######.#",
    "#........#",
    "##########",
]

wall_idx = 0
for r, row in enumerate(maze_map):
    for c, char in enumerate(row):
        if char == "#":
            name = f"Wall_{wall_idx}"
            send_cmd({"cmd": "SPAWN_NODE", "type": "MeshInstance3D", "name": name, "parent": "/root/Main/VisualScene"}, 100 + wall_idx, quiet=True)
            x, z = (c - 5) * 4, (r - 5) * 4
            send_cmd({"cmd": "SET_PROPS", "target": f"/root/Main/VisualScene/{name}", "props": {"position": [x, 2, z], "scale": [4, 4, 4]}}, 200 + wall_idx, quiet=True)
            wall_script = """extends MeshInstance3D
func _ready():
    mesh = BoxMesh.new()
    var mat = StandardMaterial3D.new()
    mat.albedo_color = Color(0.2, 0.2, 0.5)
    mat.roughness = 0.5
    mesh.material = mat
"""
            send_cmd({"cmd": "INJECT_SCRIPT", "target": f"/root/Main/VisualScene/{name}", "code": base64.b64encode(wall_script.encode()).decode()}, 300 + wall_idx, quiet=True)
            wall_idx += 1
print(f"  -> {wall_idx} walls instantiated.")

# 5. Spawn Bouncing Ball
print("\n[5] Injecting High-Altitude Bouncing Ball...")
send_cmd({"cmd": "SPAWN_NODE", "type": "MeshInstance3D", "name": "Ball", "parent": "/root/Main/VisualScene"}, 500)

ball_script = """extends MeshInstance3D
var velocity = Vector3(5, 0, 5)
var gravity = -20.0
var bounce = 0.8
var walls = []

func _ready():
    set_process(true)
    mesh = SphereMesh.new()
    mesh.radius = 1.0
    mesh.height = 2.0
    var mat = StandardMaterial3D.new()
    mat.albedo_color = Color(1, 0, 0)
    mat.emission_enabled = true
    mat.emission = Color(1, 0, 0)
    mat.emission_energy_multiplier = 0.5
    mesh.material = mat
    position = Vector3(-16, 20, -16)
    
    var map = [
        "##########", "#........#", "#.######.#", "#.#....#.#", "#.#.##.#.#",
        "#.#..#.#.#", "#.#.##.#.#", "#.######.#", "#........#", "##########"
    ]
    for r in range(10):
        for c in range(10):
            if map[r][c] == "#":
                var wx = (c - 5) * 4
                var wz = (r - 5) * 4
                walls.append(Rect2(wx - 2, wz - 2, 4, 4))

func _process(delta):
    velocity.y += gravity * delta
    var next_pos = position + velocity * delta
    if next_pos.y < 1.0:
        next_pos.y = 1.0
        velocity.y = -velocity.y * bounce
    var ball_rect = Rect2(next_pos.x - 1, next_pos.z - 1, 2, 2)
    for w in walls:
        if w.intersects(ball_rect):
            if abs(position.x - next_pos.x) > 0.001: velocity.x = -velocity.x * bounce
            if abs(position.z - next_pos.z) > 0.001: velocity.z = -velocity.z * bounce
            next_pos = position
            break
    position = next_pos
"""
send_cmd({"cmd": "INJECT_SCRIPT", "target": "/root/Main/VisualScene/Ball", "code": base64.b64encode(ball_script.encode()).decode()}, 501)

print("\n" + "="*50)
print("   🚀 ORCHESTRATION COMPLETE! Simulation is Live.")
print("   Engine is now rendering locally.")
print("="*50)

print("\nMonitoring Ball Trajectory (Ctrl+C to stop)...")
try:
    while True:
        state = send_cmd({"cmd": "GET_STATE", "target": "/root/Main/VisualScene/Ball", "props": ["position"]}, 999, quiet=True)
        if state.get("status") == "OK":
            pos = state["data"].get("position", [0, 0, 0])
            sys.stdout.write(f"\rBall Position: {list_to_vec3(pos)}    ")
            sys.stdout.flush()
        time.sleep(0.5)
except KeyboardInterrupt:
    print("\n\nMonitoring stopped.")
