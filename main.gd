extends Node3D

func _ready():
	print("VoxAI Demo: Starting internal orchestration...")
	
	# Lite Environment
	var env_node = WorldEnvironment.new()
	add_child(env_node)
	var env = Environment.new()
	env.background_mode = 2 # BG_SKY
	env.sky = Sky.new()
	env.sky.sky_material = ProceduralSkyMaterial.new()
	env_node.environment = env
	
	# Global Light
	var sun = DirectionalLight3D.new()
	add_child(sun)
	sun.rotation_degrees = Vector3(-60, 45, 0)
	sun.light_energy = 2.0
	
	# Camera
	var cam = Camera3D.new()
	add_child(cam)
	cam.position = Vector3(0, 15, 30)
	cam.rotation_degrees = Vector3(-25, 0, 0)
	cam.current = true
	
	# Neon Floor
	var floor_node = MeshInstance3D.new()
	add_child(floor_node)
	floor_node.mesh = PlaneMesh.new()
	floor_node.mesh.size = Vector2(100, 100)
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(0, 1, 0)
	mat.emission_enabled = true
	mat.emission = Color(0, 1, 0)
	mat.emission_energy_multiplier = 0.5
	floor_node.mesh.material = mat
	
	# Central Ball (Static Placeholder)
	var ball = MeshInstance3D.new()
	add_child(ball)
	ball.mesh = SphereMesh.new()
	ball.mesh.radius = 2.0
	ball.mesh.height = 4.0
	var bmat = StandardMaterial3D.new()
	bmat.albedo_color = Color(1, 0, 0)
	ball.mesh.material = bmat
	ball.position = Vector3(0, 2, 0)
	
	print("VoxAI Demo: Scenes ready.")
