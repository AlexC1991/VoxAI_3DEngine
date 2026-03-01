extends Node3D

# --- VoxAI Internal Demo Logic ---
# This script is baked into the POC to provide an immediate visual demonstration.

func _ready():
	print("VoxAI Demo: Initializing internal orchestration...")
	setup_environment()
	setup_maze()
	setup_ball()

func setup_environment():
	var env_node = WorldEnvironment.new()
	env_node.name = "Env"
	add_child(env_node)
	
	var env = Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = Sky.new()
	env.sky.sky_material = ProceduralSkyMaterial.new()
	env.tonemap_mode = Environment.TONE_MAP_FILMIC
	env.glow_enabled = true
	env.glow_bloom = 0.5
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.ambient_light_sky_contribution = 0.5
	env_node.environment = env
	
	var cam = Camera3D.new()
	cam.name = "MainCamera"
	add_child(cam)
	cam.position = Vector3(0, 25, 25)
	cam.rotation_degrees = Vector3(-45, 0, 0)
	cam.current = true
	
	var sun = DirectionalLight3D.new()
	sun.name = "Sun"
	add_child(sun)
	sun.rotation_degrees = Vector3(-60, 45, 0)
	sun.light_energy = 2.5
	sun.shadow_enabled = true

func setup_maze():
	# Neon Floor
	var floor_node = MeshInstance3D.new()
	floor_node.name = "Floor"
	add_child(floor_node)
	floor_node.mesh = PlaneMesh.new()
	floor_node.mesh.size = Vector2(40, 40)
	var fmat = StandardMaterial3D.new()
	fmat.albedo_color = Color(0, 1, 0)
	fmat.emission_enabled = true
	fmat.emission = Color(0, 1, 0)
	fmat.emission_energy_multiplier = 0.1
	floor_node.mesh.material = fmat
	
	# Walls
	var maze_map = [
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
	
	var wall_idx = 0
	for r in range(10):
		for c in range(10):
			if maze_map[r][c] == "#":
				var wall = MeshInstance3D.new()
				wall.name = "Wall_" + str(wall_idx)
				add_child(wall)
				wall.mesh = BoxMesh.new()
				var wmat = StandardMaterial3D.new()
				wmat.albedo_color = Color(0.2, 0.2, 0.5)
				wmat.roughness = 0.5
				wall.mesh.material = wmat
				wall.position = Vector3((c - 5) * 4, 2, (r - 5) * 4)
				wall.scale = Vector3(4, 4, 4)
				wall_idx += 1

func setup_ball():
	var ball = MeshInstance3D.new()
	ball.name = "Ball"
	add_child(ball)
	ball.mesh = SphereMesh.new()
	ball.mesh.radius = 1.0
	ball.mesh.height = 2.0
	var bmat = StandardMaterial3D.new()
	bmat.albedo_color = Color(1, 0, 0)
	bmat.emission_enabled = true
	bmat.emission = Color(1, 0, 0)
	bmat.emission_energy_multiplier = 0.5
	ball.mesh.material = bmat
	ball.position = Vector3(-16, 20, -16)
	
	# Attach the same logic script to the ball node
	var script = GDScript.new()
	script.source_code = """extends MeshInstance3D
var velocity = Vector3(5, 0, 5)
var gravity = -20.0
var bounce = 0.8
var walls = []

func _ready():
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
	ball.set_script(script)
