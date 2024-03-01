import Foundation
import Glibc


// this enum isn't in use yet
enum CommandType {
	case beginFire // LB
	case endFire // LB
	case lock // RB
	case toggleRCS // camera button: rcs follows crosshair / rcs disabled
	case menu // start
	case toggleDrones // A
	case beginTalk // B
	case endTalk // B
	case beginLeftRoll // X
	case endLeftRoll // X
	case beginRightRoll // Y
	case endRightRoll // Y
	case dpadup // select weapon/module 2
	case dpaddn // select weapon/module 4
	case dpadl // select weapon/module 1
	case dpadr // select weapon/module 3
	case toggleAutopilot // left stick button: toggle autopilot
	case toggleCameraDefault // right stick button: set camera direction to ship's forward direction
	case forwardThrust // right throttle
	case backwardThrust // left throttle
	case thrustVector // left stick
	case cameraVector // right stick
}

enum InterfaceMode {
	case mainMenu
	case physicsSim
	case workshop
	case flightMode
	case combat
	case travel
	case mining
	case rts
}

enum DpadDirection {
	case NONE	
	case UP
	case DOWN
	case LEFT
	case RIGHT
}

struct Action {
	let object: SphericalCow
	let force: Vector
	let torque: Vector
}

var sun = Celestial(name: "Sol", SphericalCow(id: 0,
						 referenceFrame: nil,
						 position: Vector(0, 0, 0),
						 velocity: Vector(0, 0, 0),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 1.989e30, radius: 696.34e6, frictionCoefficient: 0.8))
var mercury = Celestial(name: "Mercury", SphericalCow(id: 1,
						 referenceFrame: nil,
						 position: Vector(0, 0, -57.91e9),
						 velocity: Vector(0, 0, 47.87e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 3.285e23, radius: 2.44e6, frictionCoefficient: 0.8))
var venus = Celestial(name: "Venus", SphericalCow(id: 2,
						 referenceFrame: nil,
						 position: Vector(0, 0, 108.2e9),
						 velocity: Vector(0, 0, 35.02e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 4.867e24, radius: 6.0518e6, frictionCoefficient: 0.8))
var earth = Celestial(name: "Earth", SphericalCow(id: 3,
						 referenceFrame: nil,
						 position: Vector(0, 0, -149.6e9),
						 velocity: Vector(0, 29.78e3, 0),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 5.972e24, radius: 6.371e6, frictionCoefficient: 0.8))
var moon = Celestial(name: "Luna", SphericalCow(id: 4,
						 referenceFrame: nil,
						 position: Vector(earth.moo.position.x - 384e6, earth.moo.position.y, earth.moo.position.z),
						 velocity: Vector(earth.moo.velocity.x, earth.moo.velocity.y, earth.moo.velocity.z + 1.022e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 7.342e22, radius: 1.7371e6, frictionCoefficient: 0.2))
var player1 = Entity(name: "Player 1", SphericalCow(id: 5,
						 referenceFrame: earth.moo,
						 position: Vector(0.0, 0.0, earth.moo.radius + 1.1),
						 velocity: Vector(),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z:0),
						 spin: Vector(0.0, 0.0, 0.0),
						 mass: 1e3, radius:1.0, frictionCoefficient: 0.5))
var camera = Celestial(name: "Camera", SphericalCow(id: -1,
						  referenceFrame: earth.moo,
						  position: Vector(20.0, 100.0, earth.moo.radius + 0.5),
						  velocity: Vector(),
						  orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						  spin: Vector(0, 0, 0),
						  mass: 0, radius: 0, frictionCoefficient: 0.0))

// wanted: spatially hierarchic pseudorandom deterministic universe generator that refines mass distribution probabilities from a high level and downwards somewhat realistically in acceptable cpu/gpu time

var lights = [sun]
var celestials = [sun, mercury, venus, earth, moon]
var allTheThings: [Moo] = [sun, mercury, venus, earth, moon, player1]
//var stations = []
var ships = [player1]
//var asteroids = []

// composite[0] is the player's ship

struct GameState: Codable {
	static let gameStatePath = "composites.json"
	var composites: [CompositeCod] = []
	var interfaceModeIndex: Int = 0
	var sens: Double = 0.5 // input sensitivity
	var deadzone: Double = 0.05

	static func load(_ path: String = gameStatePath) -> GameState {
		var gameStateJSONstr: String? = nil
		if FileManager.default.fileExists(atPath: path) {
			gameStateJSONstr = try! String(contentsOfFile: path, encoding: String.Encoding.utf8)
			return try!JSONDecoder().decode(self, from:gameStateJSONstr!.data(using: .utf8)!)
		} else {
			print("could not find \(path), starting with nothing")
			return GameState()
		}
	}

	func save(_ path: String = gameStatePath) {
		do {
			//let jsonData = try JSONEncoder().encode(composites)
			let jsonData = try JSONEncoder().encode(self)
			if let jsonString = String(data: jsonData, encoding: .utf8) {
				try jsonString.write(toFile: path, atomically: true, encoding: .utf8)
				print("Saved \(path) successfully. \(composites.count) objects.")
			} else {
				print("Failed to convert JSON data to string.")
			}
		} catch {
			print("Failed to save to \(path): \(error)")
		}
	}
}

var s: GameState = GameState();
var enableGrid = false
var grid: [CompositeCod] = []
var gridrefs: [Objref] = []
let grid_size = 10000.0
var objrefs: [Objref] = []
var actions: [Action] = []
var interfaceMode: InterfaceMode = .physicsSim
//var interfaceMode: InterfaceMode = .flightMode
var buttonPresses: Int32 = 4
var curcom: Int = 0 // current composite
var curbox: Int = 0 // current boxoid
var faceIndex: Int = 0
var pickedPart: CompositeCod = CompositeCod.unit()
var partsPickerIsOpen = false
var dpad: DpadDirection = .NONE
var controller: OpaquePointer? = nil
var shouldExit = false
var rcsIsEnabled = true
let worldUpVector = Vector(0, 1, 0) // x is right, y is up, z is backward (right-handed coordinate system)
var thrustVector = Vector(0, 0, 0)
var cameraSpherical = SphericalVector(1, 0, 0)

let totalTime = 1e12
var dt = 0.01
var t = 0.0

/*
Mercury	(0.101961, 0.101961, 0.101961)
Venus	(0.901961, 0.901961, 0.901961)
Earth	(0.184314, 0.415686, 0.411765)
Mars	(0.6, 0.239216, 0)
Jupiter	(0.690196, 0.498039, 0.207843)
Saturn	(0.690196, 0.560784, 0.211765)
Uranus	(0.341176, 0.313725, 0.666667)
Neptune	(0.211765, 0.407843, 0.588235)
*/
func main() {
	let materialsArray = UnsafeMutablePointer<material>.allocate(capacity: 7)
	// Sun
	materialsArray[0].diffuse = (1.0, 1.0, 0.95)
	materialsArray[0].emissive = (4.38e24, 4.38e24, 4.18e24)
	//Mercury	
	materialsArray[1].diffuse = (0.101961, 0.101961, 0.101961)
	materialsArray[1].emissive = (0, 0, 0)
	//Venus	
	materialsArray[2].diffuse = (0.901961, 0.901961, 0.901961)
	materialsArray[2].emissive = (0, 0, 0)
	//Earth	
	materialsArray[3].diffuse = (0.184314, 0.415686, 0.411765)
	materialsArray[3].emissive = (0, 0, 0)
	//Moon
	materialsArray[4].diffuse = (0.8, 0.8, 0.8)
	materialsArray[4].emissive = (0, 0, 0)
	//Player1
	materialsArray[5].diffuse = (0.4, 0.4, 1.0)
	materialsArray[5].emissive = (0.4, 0.4, 1.0)
	//trajectory
	materialsArray[6].diffuse = (0.0, 0.0, 0.0)
	materialsArray[6].emissive = (50.0, 0.0, 0.0)

	//s.load()
	s = GameState.load()
	if s.composites.count == 0 {
		s.composites.append(CompositeCod.unit())
		//s.composites[0].b.append(BoxoidCod.unit())
		//s.composites[0].b[0].translate(Vector(0, 1.0, 0.5))
		//s.composites[0].bbox = s.composites[0].calculateBBox()
		s.composites[0].position = player1.moo.position
		s.composites[0].orientation = player1.moo.orientation
		//s.composites.append(CompositeCod.load("grid.json"))
		//s.composites[1].orientation = Quaternion()
	} else {
		player1.moo.position = s.composites[0].position
		player1.moo.orientation = s.composites[0].orientation
	}
	player1.c = s.composites[0]
	player1.createCows()
//	player1.recomputeCows()
	player1.updateCows()

	if(s.composites.count < 4)
	{
		for i in 0 ..< 250 {
			let radius = pow(Double.random(in: 0.4 ..< 1.0), 3.0)
			let mass = pow(radius, 3.0) * 1000.0

//			let mass = Double.random(in: 1.0 ..< 1000.0)
//			let radius = pow(mass * 0.001, 0.33)

			var ent = Entity(name: String(format: "ent%d", i), SphericalCow(id: 6 + Int64(i),
				 referenceFrame: earth.moo,
				 position: Vector(Double.random(in: -10.0 ..< 10.0), Double.random(in: -10.0 ..< 10.0), earth.moo.radius + Double.random(in: 1.0 ..< 100.0)),
				 velocity: Vector(),
				 orientation: Quaternion(w: 0, x: 0, y: 1, z:0),
				 spin: Vector(0.1, 0.0, 0.0),
				 mass: mass, radius: radius, frictionCoefficient: 0.5))
			s.composites.append(CompositeCod.unit())
			s.composites[i + 1].position = ent.moo.position
			s.composites[i + 1].orientation = ent.moo.orientation
			s.composites[i + 1].scale = Float(radius * 0.6)
			ent.c = s.composites[i + 1]
			ent.createCows()
//			ent.recomputeCows()
			ent.updateCows()
			ships.append(ent)
			allTheThings.append(ent)
		}
	}

	for i in 0..<celestials.count {
		celestials[i].moo.hp = celestials[i].moo.mass * 100.0
	}

	grid.append(CompositeCod.generateGrid(grid_size * 10.0))
	//let availableModes: [InterfaceMode] = [.flightMode, .workshop]
	let availableModes: [InterfaceMode] = [.workshop, .physicsSim, .flightMode]
	//interfaceMode = availableModes[s.interfaceModeIndex % availableModes.count]
	

	startRenderer()
	SDL_Init(SDL_INIT_GAMECONTROLLER)
   

	// main game loop. handle input, do a physics tick, send results to renderer
	while( !shouldExit ) {
		// take a nap first, don't want to work too hard
		usleep(8000)
		
		// poll for inputs, generate actions
		for i in 0..<SDL_NumJoysticks() {
			if isGameController(i) {
				controller = SDL_GameControllerOpen(i)
				if controller != nil {
					var event = SDL_Event()
					let TMAX = Double(SDL_JOYSTICK_AXIS_MAX)
					while SDL_PollEvent(&event) != 0 {
						switch event.type {
						case SDL_CONTROLLERBUTTONDOWN.rawValue:
							if(interfaceMode == .physicsSim) {
								print("\(String(cString: getStringForButton(event.cbutton.button)!)) pressed")
								if		event.cbutton.button == SDL_CONTROLLER_BUTTON_Y.rawValue {
									dt *= 10.0
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_X.rawValue {
									dt *= 0.1
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_B.rawValue {
									buttonPresses += 1;
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_A.rawValue {
									buttonPresses -= 1;
								}
							} else if interfaceMode == .flightMode {
								print("\(String(cString: getStringForButton(event.cbutton.button)!)) pressed")
								if		event.cbutton.button == SDL_CONTROLLER_BUTTON_Y.rawValue {
									player1.moo.w *= 1.25
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_X.rawValue {
									player1.moo.w *= 0.8
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_B.rawValue {
									player1.moo.FTL = false
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_A.rawValue {
									if( !player1.moo.FTL) {
										player1.moo.warpVector = Vector();
										player1.moo.FTL = true
									}
								}
							} else if interfaceMode == .workshop {
 								print("\(String(cString: getStringForButton(event.cbutton.button)!)) pressed")
								if event.cbutton.button == SDL_CONTROLLER_BUTTON_Y.rawValue {
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_X.rawValue {
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_B.rawValue {
									player1.c.b[curbox] = BoxoidCod.unit()
									player1.createCows()
									player1.recomputeCows()
									player1.updateCows()
									s.composites[curcom] = player1.c
									updateComposite(objrefs[curcom], toC(s.composites[curcom]))
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_A.rawValue {
									player1.c.b.append(s.composites[curcom].b[curbox])
									player1.createCows()
									player1.recomputeCows()
									player1.updateCows()
									s.composites[curcom] = player1.c
									updateComposite(objrefs[curcom], toC(s.composites[curcom]))
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER.rawValue {
									faceIndex = ((faceIndex + 1) % 6)
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER.rawValue {
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP.rawValue {
									dpad = .UP
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN.rawValue {
									dpad = .DOWN
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT.rawValue {
									dpad = .LEFT
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT.rawValue {
									dpad = .RIGHT
								}
							}
						case SDL_CONTROLLERBUTTONUP.rawValue:
							if event.cbutton.button == SDL_CONTROLLER_BUTTON_START.rawValue {
								shouldExit = true
							} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK.rawValue {
								// cycle through game modes
								interfaceMode = availableModes[++s.interfaceModeIndex % availableModes.count]
								rcsIsEnabled = (interfaceMode == .flightMode)
							}
						case SDL_CONTROLLERAXISMOTION.rawValue:
							var val = Double(event.caxis.value) / TMAX
							if(abs(val) < s.deadzone) {
								val = 0
							} else if val > 0 {
								val -= s.deadzone
							} else {
								val += s.deadzone
							}
							if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTX.rawValue) {
								thrustVector.x = s.sens * val
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTY.rawValue) {
								thrustVector.y = s.sens * val
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERLEFT.rawValue) {
								thrustVector.z = s.sens * -val
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERRIGHT.rawValue) {
								thrustVector.z = s.sens * val
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTX.rawValue) {
								cameraSpherical.theta = s.sens * -val
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTY.rawValue) {
								cameraSpherical.phi = s.sens * val
							}
						default:
							break
						}
					}
				} else {
					let errorMessage = String(cString: SDL_GetError())
					print("Could not open game controller: \(errorMessage)")
				}
			}
		}

		// update game state
		
		if objrefs.count == 0 {
			for (i, composite) in s.composites.enumerated() {
				var ccod = composite
				ccod.position -= camera.moo.position
				let c = toC(ccod)
				objrefs.append(submitComposite(c))
				print("new object ", objrefs[i])
			}
			var ccod = grid[0]
			ccod.position -= camera.moo.position
			let c = toC(ccod)
			gridrefs.append(submitComposite(c))
			print("new object: grid")
		}
		if interfaceMode == .physicsSim || interfaceMode == .flightMode {
			if(rcsIsEnabled) {
				var q = Quaternion(pitch: cameraSpherical.theta, yaw: -thrustVector.z, roll: cameraSpherical.phi)
				q = player1.moo.orientation * q * player1.moo.orientation.conjugate
				let torque = q.xyz * 10000.0
				var f = Quaternion(w: 0, x: thrustVector.x * 1e6, y: 0, z: thrustVector.y * 1e6)
				f = player1.moo.orientation * f * player1.moo.orientation.conjugate
				actions = [Action(
					object: player1.moo,
					force: f.xyz,
					torque: torque)]
			} else {
				actions = []
			}
			gravTick(center: sun.moo, celestials: &celestials, t: t, dt: dt)
			tick(actions: actions, entities: &ships, celestials: &celestials, t: t, dt: dt)
			for (i, ship) in ships.enumerated() {
			   if(ship.dinged > 0) {
					if(ship.damageReport()) {
						s.composites[i] = ship.c
						updateComposite(objrefs[i], toC(s.composites[i]))
					}
					ship.dinged = 0
				}	 
				if(player1.moo.hp < -1000000000.001) {
					dt = 0.0
					print("game over! to respawn: rm composites.json; ./subparcollider")
					shouldExit = true
					let encoder = JSONEncoder()
					encoder.outputFormatting = .prettyPrinted
					//try! print((String(data: encoder.encode(player1), encoding: .utf8)!)
				}
				s.composites[i].position = ship.moo.position
				s.composites[i].orientation = ship.moo.orientation
			}
			t += dt
		} else if interfaceMode == .workshop {
			switch(dpad) {
				case .RIGHT:
					// Elongaaaate
					if(thrustVector.length > 0.05) {
						s.composites[curcom].b[curbox].elongate(thrustVector * 0.01)
						updateComposite(objrefs[curcom], toC(s.composites[curcom]))
						s.composites[curcom].bbox = s.composites[curcom].calculateBBox()
						player1.recomputeCows()
						player1.moo.radius = player1.moo.radius * 0.95 + 0.05 * s.composites[curcom].bbox.halfsize
					}
				case .LEFT:
					// Bouba vs Kiki
					if(thrustVector.length > 0.05) {
						s.composites[curcom].b[curbox].bulgerize(thrustVector * 0.01)
						updateComposite(objrefs[curcom], toC(s.composites[curcom]))
						s.composites[curcom].bbox = s.composites[curcom].calculateBBox()
						player1.recomputeCows()
						player1.moo.radius = player1.moo.radius * 0.95 + 0.05 * s.composites[curcom].bbox.halfsize
					}
				case .UP:
					// move
				if(thrustVector.length > 0.05) {
						s.composites[curcom].b[curbox].translate(thrustVector * 0.01)
						updateComposite(objrefs[curcom], toC(s.composites[curcom]))
						s.composites[curcom].bbox = s.composites[curcom].calculateBBox()
						player1.recomputeCows()
						player1.moo.radius = player1.moo.radius * 0.95 + 0.05 * s.composites[curcom].bbox.halfsize
					}	
				default:
					break;
			}
		}
		grid[0].position = player1.moo.position
		grid[0].position.round(grid_size)
		if(player1.moo.referenceFrame != nil) {
			grid[0].position = (player1.moo.position - player1.moo.referenceFrame!.position)
			grid[0].position.round(grid_size)
			grid[0].position += player1.moo.referenceFrame!.position
		}

		// camera and rendering
		var renderMisc = render_misc()
		renderMisc.buttonPresses = abs(buttonPresses);
		renderMisc.materials = materialsArray
		let cameraTarget = player1.moo
		let nearestCelestial = earth.moo
		let relativeVelocity = cameraTarget.velocity - nearestCelestial.velocity
		var prograde = relativeVelocity
		camera.moo.position = Vector(0.0, 80.0, earth.moo.radius + 20.0)
		//camera.moo.position = Vector(10.0, 10.0, earth.moo.radius + 100.0)
		camera.moo.referenceFrame = earth.moo
//		camera.moo.position = cameraTarget.position + Vector(2.0, 10.0, 2.0)
		//camera.moo.position = cameraTarget.position + Vector(10, 10, -4.5 * cameraTarget.radius)
		if(relativeVelocity.lengthSquared == 0) {
			prograde = cameraTarget.orientation * Vector(0, 0, 1)
		} else {
			prograde = relativeVelocity.normalized()
			//camera.moo.position = cameraTarget.position + relativeVelocity.normalized() * -4.5 * cameraTarget.radius
		}
		var camFwd = Vector(0.0, -0.8, -0.2)
		//var camFwd = Vector(0.0, -0.1, -0.9)
		//var camFwd = (cameraTarget.position - camera.moo.position).normalized()
		var upVec = cameraTarget.position.normalized()
		let pitchAxis = camFwd.cross(upVec).normalized()
		let pitchQuat = Quaternion(axis: pitchAxis, angle: 2.0 * cameraSpherical.phi * (rcsIsEnabled ? 0.0 : 1.0))
		let yawQuat = Quaternion(axis: upVec, angle: 2.0 * cameraSpherical.theta * (rcsIsEnabled ? 0.0 : 1.0))
		if(interfaceMode == .flightMode) {
			camFwd = cameraTarget.orientation * Vector(0, 0, -1)
			upVec = cameraTarget.orientation * worldUpVector
			camera.moo.position = cameraTarget.position + camFwd * -4.5 * cameraTarget.radius
		} else {
			upVec = camera.moo.position.normalized()
			//camFwd = yawQuat * pitchQuat * camFwd
			//upVec = pitchQuat * upVec
		}
		renderMisc.camForward = (Float(camFwd.x), Float(camFwd.y), Float(camFwd.z))
		//renderMisc.camUp = (Float(upVec.z), Float(upVec.y), Float(upVec.x))
		renderMisc.camUp = Vector(0, 0, 1).float
		// camera is at 0,0,0 to make it easy for the renderer
		renderMisc.camPosition = (0, 0, 0)
		for (index, object) in lights.enumerated() {
			if(index >= MAX_LIGHTS){
				print("warning: MAX_LIGHTS exceeded")
				break
			}
			withUnsafeMutablePointer(to:&renderMisc.lights) { lights_ptr in
				lights_ptr[index].position = (object.moo.position - (camera.moo.position + camera.moo.referenceFrame!.position)).float
//				lights_ptr[index].position =
//					(Float(object.moo.position.x - camera.moo.position.x),
//					 Float(object.moo.position.y - camera.moo.position.y),
//					 Float(object.moo.position.z - camera.moo.position.z))
				// hardcoded values for the sun, improve later
				lights_ptr[index].color = (4.38e24, 4.38e24, 4.18e24)
			}
		}
	

		// we have to sort the things before we send them to the renderer, otherwise transparency breaks.
		allTheThings.sort(by: { ($0.moo.position - camera.moo.position).lengthSquared > ($1.moo.position - camera.moo.position).lengthSquared })
		var trajectory: [Vector] = []
		//var trajectory = extrapolateTrajectory(subject: cameraTarget, relativeTo: nearestCelestial, otherObjects: allTheThings, t: t, dt: 10.0, iterations: 500)
		trajectory.sort(by: { ($0 - camera.moo.position).lengthSquared > ($1 - camera.moo.position).lengthSquared })
		//print("cameraTarget: \(cameraTarget.position)\ntrajectory: \(trajectory)\n")
		//let sphereArray = UnsafeMutablePointer<sphere>.allocate(capacity: allTheThings.count)
		let sphereArray = UnsafeMutablePointer<Sphere>.allocate(capacity: celestials.count + trajectory.count)
		for (index, object) in celestials.enumerated() {
			// the mapping from object.id to material_idx should not be 1:1 in the future but it's good enough for now
			// center everything on the camera before converting to float to avoid float precision issues when rendering
			if(object.moo === camera.moo.referenceFrame) {
				let pos = -camera.moo.position
				sphereArray[index + trajectory.count] = Sphere(position: pos.float,
										radius: Float(object.moo.radius),
										material_idx: (Int32(object.moo.id)))
				
			} else {
				let pos = object.moo.position - (camera.moo.referenceFrame!.position + camera.moo.position)
				sphereArray[index + trajectory.count] = Sphere(position: pos.float,
											radius: Float(object.moo.radius),
											material_idx: (Int32(object.moo.id)))
			}
		}
		for (index, object) in trajectory.enumerated() {
			// center everything on the camera before converting to float to avoid float precision issues when rendering
			let position = Vector(object.x - camera.moo.position.x, object.y - camera.moo.position.y, object.z - camera.moo.position.z)
			sphereArray[index] = Sphere(position: position.float,
										radius: Float(player1.moo.radius * pow(position.length * 0.01, 0.9)),
										material_idx: 6)
		}
		let compositeArray = UnsafeMutablePointer<Objref>.allocate(capacity: objrefs.count + 1)
		for (index, object) in objrefs.enumerated() {
			var position = s.composites[object.id].position - camera.moo.position
			compositeArray[index] = Objref(orientation: s.composites[object.id].orientation.float, position: position.float, id: object.id, type: object.type)
		}
		var position = grid[0].position - camera.moo.position
		compositeArray[objrefs.count] = Objref(orientation: grid[0].orientation.float, position: position.float, id: gridrefs[0].id, type: gridrefs[0].type)
		//for i in 0..<27 {
		//	var position = grid[0].position + (Vector(Double((i/9) - 1), Double(((i/3) % 3) - 1), Double(((i % 9) % 3) - 1)) * (grid_size * 4)) - camera.moo.position
		//	compositeArray[i + objrefs.count] = Objref(orientation: grid[0].orientation.float, position: position.float, id: gridrefs[0].id, type: gridrefs[0].type)
		//}

		//for i in 0..<30 {
		//	var position = grid[0].position - camera.moo.position
		//	if(i < 10) {
		//		position.x += Double((i%10) - 5) * (grid_size * 12) 
		//	} else if(i < 20) {
		//		position.y += Double((i%10) - 5) * (grid_size * 12) 
		//	} else {
		//		position.z += Double((i%10) - 5) * (grid_size * 12) 
		//	}
		//	compositeArray[i + 27 + objrefs.count] = Objref(orientation: grid[0].orientation.float, position: position.float, id: gridrefs[0].id, type: gridrefs[0].type)
		//}

//		for i in 0..<125 {
//			var position = grid[0].position + (Vector(Double((i/25) - 2), Double(((i/5) % 5) - 2), Double(((i % 25) % 5) - 2)) * (grid_size * 4)) - camera.moo.position
//			compositeArray[i + objrefs.count] = Objref(orientation: grid[0].orientation.float, position: position.float, id: gridrefs[0].id, type: gridrefs[0].type)
//		}

		if(enableGrid) {
			render(compositeArray, objrefs.count + 1, sphereArray, celestials.count + trajectory.count, renderMisc)
		} else {
			render(compositeArray, objrefs.count, sphereArray, celestials.count + trajectory.count, renderMisc)
		}
		compositeArray.deallocate()
		sphereArray.deallocate()
	}
	// end main game loop

	s.save()

	SDL_Quit()
	stopRenderer()
	materialsArray.deallocate()
}

main()







/*
The value of 3.828 x 10^26 W is the total power emitted by the sun in all directions - this is its luminosity. To convert this to a power per unit solid angle (W/sr), we need to divide by the total solid angle of a sphere, which is 4π steradians.

For the visible part, that's approximately 43.4% of the total power, or 1.66 x 10^26 W. Dividing by 4π gives us about 1.32 x 10^25 W/sr.

If we split this equally among the R, G, and B components (assuming for simplicity that they're all of equal intensity, which isn't quite true but is close enough for a first approximation), each one gets about 4.4 x 10^24 W/sr.

In reality, the sun emits slightly more green light than red or blue, so if we account for that by assigning the given RGB values of 255, 255, 244 to R, G, and B respectively, we get:

R: 4.38 x 10^24 W/sr
G: 4.38 x 10^24 W/sr
B: 4.18 x 10^24 W/sr

Radio: The sun's flux density at a frequency of 1 GHz is about 10^-22 W m^-2 Hz^-1. Considering the entire radio spectrum, the total power is very small compared to other forms of radiation.

Microwave: The sun's contribution to the Cosmic Microwave Background (CMB) radiation is minuscule. The CMB has a nearly uniform temperature of about 2.725 K, and the sun's motion adds a dipole anisotropy of only about 3 mK.

Infrared: About 49.4% of the sun's total power output, or approximately 1.89 x 10^26 W, is emitted as infrared radiation.

Visible: About 43.4% of the sun's total power output, or approximately 1.66 x 10^26 W, is emitted as visible light.

Ultraviolet: About 7.1% of the sun's total power output, or approximately 2.72 x 10^25 W, is emitted as ultraviolet radiation.

X-rays: The sun's X-ray flux is highly variable and depends on solar activity, but it's estimated to be around 10^-6 W/m^2 during a solar flare. Over the entire surface of the sun, this would be about 10^20 W, which is still many orders of magnitude less than the total power output.

Gamma rays: The sun produces essentially no gamma rays, because the temperatures in the sun's core are not high enough to support the nuclear reactions that produce gamma rays.


wide spectrum rendering:
radio: yellow
microwave: orange
infrared: red
visible light: greyscale
ultraviolet: purple
x-ray: blue
gamma: green

*/
