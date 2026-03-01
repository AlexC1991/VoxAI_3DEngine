import socket
import json
import base64
import time

def send_cmd(cmd, id=1):
    packet = {"id": id, "key": "VOXAI_MASTER_BRAIN"}
    packet.update(cmd)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5.0)
    try:
        s.connect(("127.0.0.1", 5555))
        s.sendall((json.dumps(packet) + "\n").encode("utf-8"))
        data = b""
        while not data.endswith(b"\n"):
            chunk = s.recv(4096)
            if not chunk: break
            data += chunk
        s.close()
        return json.loads(data.decode("utf-8"))
    except Exception as e:
        return {"status": "ERROR", "error": str(e)}

print("--- VoxAI 3D Maze Orchestration ---")

# 1. Reset Scene
send_cmd({"cmd": "FREE_NODE", "target": "/root/Main/VisualScene"}, 0)
send_cmd({"cmd": "SPAWN_NODE", "type": "Node3D", "name": "VisualScene", "parent": "/root/Main"}, 1)

# 2. Setup Environment & Camera
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
print("[3] Constructing Maze Walls...")
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
            send_cmd({"cmd": "SPAWN_NODE", "type": "MeshInstance3D", "name": name, "parent": "/root/Main/VisualScene"}, 100 + wall_idx)
            
            # Position = (c - grid_size/2) * wall_size
            x = (c - 5) * 4
            z = (r - 5) * 4
            
            send_cmd({"cmd": "SET_PROPS", "target": f"/root/Main/VisualScene/{name}", "props": {
                "position": [x, 2, z],
                "scale": [4, 4, 4]
            }}, 200 + wall_idx)
            
            wall_script = """extends MeshInstance3D
func _ready():
    mesh = BoxMesh.new()
    var mat = StandardMaterial3D.new()
    mat.albedo_color = Color(0.2, 0.2, 0.5)
    mat.roughness = 0.5
    mesh.material = mat
"""
            send_cmd({"cmd": "INJECT_SCRIPT", "target": f"/root/Main/VisualScene/{name}", "code": base64.b64encode(wall_script.encode()).decode()}, 300 + wall_idx)
            wall_idx += 1

# 5. Spawn Bouncing Ball with Collision Logic
print("[4] Spawning Ball with Maze Logic...")
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
    
    # Simple bounding boxes for the walls based on the maze map
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
    
    # Plane Floor
    if next_pos.y < 1.0:
        next_pos.y = 1.0
        velocity.y = -velocity.y * bounce
        
    # simulated Wall collisions (Top-down Rect checks)
    var ball_rect = Rect2(next_pos.x - 1, next_pos.z - 1, 2, 2)
    for w in walls:
        if w.intersects(ball_rect):
            # Collision detected. Simple bounce back based on entry direction
            if abs(position.x - next_pos.x) > 0.001:
                velocity.x = -velocity.x * bounce
            if abs(position.z - next_pos.z) > 0.001:
                velocity.z = -velocity.z * bounce
            next_pos = position # Bounce logic simplified: stay at current pos this frame
            break
            
    position = next_pos
"""
send_cmd({"cmd": "INJECT_SCRIPT", "target": "/root/Main/VisualScene/Ball", "code": base64.b64encode(ball_script.encode()).decode()}, 501)

print("\nMaze created. Ball is dropping!")
time.sleep(15)
# Visualization complete.
