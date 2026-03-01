extends Node3D

var ball: MeshInstance3D
var velocity: float = 0.0
var gravity: float = -20.0
var bounce: float = 0.85

func _ready():
	print("VoxAI Demo: Starting internal orchestration with physics...")
	
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
	
	# Animated Ball
	ball = MeshInstance3D.new()
	add_child(ball)
	ball.name = "Ball"
	ball.mesh = SphereMesh.new()
	ball.mesh.radius = 1.0
	ball.mesh.height = 2.0
	var bmat = StandardMaterial3D.new()
	bmat.albedo_color = Color(1, 0, 0)
	ball.mesh.material = bmat
	ball.position = Vector3(0, 15, 0)
	
	print("VoxAI Demo: Initialization complete. Ball should be bouncing.")

func _process(delta):
	# Simple internal physics loop for the demo ball
	if ball:
		velocity += gravity * delta
		ball.position.y += velocity * delta
		
		# Floor bounce at Y=1.0 (radius=1)
		if ball.position.y < 1.0:
			ball.position.y = 1.0
			velocity = -velocity * bounce
			# Stop micro-bounces
			if abs(velocity) < 1.0:
				velocity = 0
				# Reset for infinite demo if it stops
				if ball.position.y == 1.0:
					ball.position.y = 15.0
					velocity = 0
