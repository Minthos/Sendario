// Subpar Collider.
// It's a ahem "physics" "engine"
// It calculates collisions and stuff.

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

var sun = SphericalCow(id: 0,
						 position: Vector(0, 0, 0),
						 velocity: Vector(0, 0, 0),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 1.989e30, radius: 696.34e6, frictionCoefficient: 0.8)
var mercury = SphericalCow(id: 1,
						 position: Vector(0, 0, -57.91e9),
						 velocity: Vector(0, 0, 47.87e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 3.285e23, radius: 2.44e6, frictionCoefficient: 0.8)
var venus = SphericalCow(id: 2,
						 position: Vector(0, 0, 108.2e9),
						 velocity: Vector(0, 0, 35.02e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 4.867e24, radius: 6.0518e6, frictionCoefficient: 0.8)
var earth = SphericalCow(id: 3,
						 position: Vector(0, 0, -149.6e9),
						 velocity: Vector(0, 29.78e3, 0),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 5.972e24, radius: 6.371e6, frictionCoefficient: 0.0001)
var moon = SphericalCow(id: 4,
						 position: Vector(earth.position.x - 384e6, earth.position.y, earth.position.z),
						 velocity: Vector(earth.velocity.x, earth.velocity.y, earth.velocity.z + 1.022e3),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						 spin: Vector(0, 0, 0),
						 mass: 7.342e22, radius: 1.7371e6, frictionCoefficient: 0.8)
var player1 = SphericalCow(id: 5,
						 position: Vector(moon.position.x, moon.position.y, moon.position.z + moon.radius + 200000.1),
						 velocity: Vector(moon.velocity.x, moon.velocity.y + 1600.0, moon.velocity.z),
						 orientation: Quaternion(w: 0, x: 0, y: 1, z:0),
						 spin: moon.spin,
						 mass: 10e3, radius:1.0, frictionCoefficient: 0.5)
var camera = SphericalCow(id: -1,
						  position: Vector(earth.position.x, earth.position.y + earth.radius * 2.0, earth.position.z - earth.radius * 8.0),
						  velocity: earth.velocity,
						  orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
						  spin: Vector(0, 0, 0),
						  mass: 0, radius: 0, frictionCoefficient: 0.0)

// wanted: spatially hierarchic pseudorandom deterministic universe generator that refines mass distribution probabilities from a high level and downwards somewhat realistically in acceptable cpu/gpu time

struct BoxoidCod: Codable {
	var corners: [Vector]
	var curvature: [Float]
	var material_idx: Int = 0 // placeholder
	var missing_faces: UInt = 0

	func elongationFactor(_ x: Double) -> Double {
		if(x > 0){
			return 1.0 + x
		} else {
			return 1.0 / (1.0 - x)
		}
	}

	mutating func elongate(_ v: Vector) {
		let vx = Vector(elongationFactor(v.x), elongationFactor(v.y),  elongationFactor(v.z))
		for i in 0..<8 {
			corners[i] = corners[i] * vx
		}
	}

	var bbox: BBox {
		return BBox(corners)
	}

	static func unit() -> BoxoidCod {
		return BoxoidCod(corners:[Vector((-1.0,  1.0, 1.0)),
							Vector((-1.0, -1.0, 1.0)),
							Vector((1.0, -1.0, 1.0)),
							Vector((1.0,  1.0, 1.0)),
							Vector((-1.0,  1.0, -1.0)),
							Vector((-1.0, -1.0, -1.0)),
							Vector((1.0, -1.0, -1.0)),
							Vector((1.0,  1.0, -1.0))],
				  curvature:[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
				  material_idx: 4, missing_faces: 0)
	}
}

func convertCToSwift(boxoid: inout Boxoid) -> BoxoidCod {
    var corners: [Vector] = []
    var curvature: [Float] = []

    withUnsafePointer(to: &boxoid.corners) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 24) { $0 }
        for i in 0..<8 {
            let x = baseAddress[i*3]
            let y = baseAddress[i*3+1]
            let z = baseAddress[i*3+2]
            corners.append(Vector(Double(x), Double(y), Double(z)))
        }
    }

    withUnsafePointer(to: &boxoid.curvature) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 12) { $0 }
        curvature = Array(UnsafeBufferPointer(start: baseAddress, count: 12))
    }

    return BoxoidCod(corners: corners, curvature: curvature, material_idx: Int(boxoid.material_idx), missing_faces: UInt(boxoid.missing_faces))
}

func convertSwiftToC(boxoidCod: BoxoidCod) -> Boxoid {
    var boxoid = Boxoid()
    withUnsafeMutablePointer(to: &boxoid.corners) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 24) { $0 }
        for i in 0..<8 {
            baseAddress[i*3] = Float(boxoidCod.corners[i].x)
            baseAddress[i*3+1] = Float(boxoidCod.corners[i].y)
            baseAddress[i*3+2] = Float(boxoidCod.corners[i].z)
        }
    }
    withUnsafeMutablePointer(to: &boxoid.curvature) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 12) { $0 }
        for i in 0..<12 {
            baseAddress[i] = Float(boxoidCod.curvature[i])
        }
    }
    boxoid.material_idx = Int32(boxoidCod.material_idx)
    boxoid.missing_faces = UInt32(boxoidCod.missing_faces)
    return boxoid
}

struct CompositeCod: Codable {
	var orientation: Quaternion
	var position: Vector
	var scale: Float
	var b: [BoxoidCod]
	
	var bbox: BBox {
		if(b.count == 0) {
			return BBox(center: self.position, halfsize: 0)
		}
		var bbox = b[0].bbox
		for i in 1..<b.count {
			bbox = bbox.union(b[i].bbox)
		}
		bbox.center += self.position
		return bbox
	}

	static func unit() -> CompositeCod {
		return CompositeCod(orientation: player1.orientation, position: player1.position, scale: 1.0, b:[BoxoidCod.unit()])
	}
}

func fromC(_ composite: inout Composite) -> CompositeCod {
    var boxoids: [BoxoidCod] = []
    for i in 0..<composite.nb {
        boxoids.append(convertCToSwift(boxoid: &composite.b[i]))
    }
    var orientation = Quaternion(w: 0, x: 0, y: 0, z: 0)
    var position = Vector(0, 0, 0)
    withUnsafePointer(to: &composite.orientation) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 4) { $0 }
        orientation = Quaternion(w: Double(baseAddress[0]), x: Double(baseAddress[1]), y: Double(baseAddress[2]), z: Double(baseAddress[3]))
    }
    withUnsafePointer(to: &composite.position) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 3) { $0 }
        position = Vector(Double(baseAddress[0]), Double(baseAddress[1]), Double(baseAddress[2]))
    }
    return CompositeCod(orientation: orientation, position: position, scale: composite.scale, b: boxoids)
}

func toC(_ compositeCod: CompositeCod) -> Composite {
    var composite = Composite()
    composite.b = UnsafeMutablePointer<Boxoid>.allocate(capacity: Int(compositeCod.b.count))
    for i in 0..<compositeCod.b.count {
        composite.b[Int(i)] = convertSwiftToC(boxoidCod: compositeCod.b[Int(i)])
    }
    withUnsafeMutablePointer(to: &composite.orientation) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 4) { $0 }
        baseAddress[0] = Float(compositeCod.orientation.w)
        baseAddress[1] = Float(compositeCod.orientation.x)
        baseAddress[2] = Float(compositeCod.orientation.y)
        baseAddress[3] = Float(compositeCod.orientation.z)
    }
    withUnsafeMutablePointer(to: &composite.position) { ptr in
        let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 3) { $0 }
        baseAddress[0] = Float(compositeCod.position.x)
        baseAddress[1] = Float(compositeCod.position.y)
        baseAddress[2] = Float(compositeCod.position.z)
    }
    composite.scale = Float(compositeCod.scale)
    composite.nb = size_t(compositeCod.b.count)
    return composite
}

struct Spaceship: Codable {
	var moo: SphericalCow
	var c: CompositeCod
}

var lights = [sun]
var allTheThings = [sun, mercury, venus, earth, moon, player1]
// composite[0] is the player's ship
// composite[1] is the workshop (object space = world space)
// composite[2] is the design being worked on
// composite[3] is gui overlay (object space = clip space)

// TODO: JSON SAVING AND LOADING IS REQUIRED FOR THESE FEATURES, GET IT DONE COMMANDER
let gameStatePath = "composites.json"
var gameStateJSONstr: String? = nil
if FileManager.default.fileExists(atPath: gameStatePath) {
    gameStateJSONstr = try! String(contentsOfFile: gameStatePath, encoding: String.Encoding.utf8)
} else {
    gameStateJSONstr = "[]"
    print("could not find composites.json, starting with nothing")
}
var composites: [CompositeCod] = try!JSONDecoder().decode([CompositeCod].self, from:gameStateJSONstr!.data(using: .utf8)!)

func saveGameState(_ path: String = gameStatePath) {
	do {
		let jsonData = try JSONEncoder().encode(composites)
		if let jsonString = String(data: jsonData, encoding: .utf8) {
			try jsonString.write(toFile: gameStatePath, atomically: true, encoding: .utf8)
			print("Saved composites.json successfully. \(composites.count) objects.")
		} else {
			print("Failed to convert JSON data to string.")
		}
	} catch {
		print("Failed to save composites.json: \(error)")
	}
}

//var composites: [CompositeCod] = []
var objrefs: [Objref] = []
var actions: [Action] = []
//var interfaceMode: InterfaceMode = .physicsSim
var interfaceMode: InterfaceMode = .workshop
var interfaceModeIndex: Int = 0
var buttonPresses: Int32 = 0
var curcom: Int = 0 // current composite
var curbox: Int = 0 // current boxoid
var faceIndex: Int = 0
var pickedPart: CompositeCod = CompositeCod.unit()
var partsPickerIsOpen = false
var sens: Double = 1 // input sensitivity
var dpad: DpadDirection = .NONE
var controller: OpaquePointer? = nil
var shouldExit = false
var rcsIsEnabled = false
let worldUpVector = Vector(0, 1, 0) // x is right, y is up, z is backward (right-handed coordinate system)
var thrustVector = Vector(0, 0, 0)
var cameraSpherical = SphericalVector(1, 0, 0)


let totalTime = 1e12
var dt = 0.0001
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
	let materialsArray = UnsafeMutablePointer<material>.allocate(capacity: allTheThings.count + 1)
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
	materialsArray[5].diffuse = (1.0, 0.0, 1.0)
	materialsArray[5].emissive = (500.0, 500.0, 15.0)
	//trajectory
	materialsArray[6].diffuse = (0.0, 0.0, 0.0)
	materialsArray[6].emissive = (50.0, 0.0, 0.0)

	if composites.count == 0 {
		composites.append(CompositeCod.unit())
	}
	

	startRenderer()
	SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO)
   

	// main game loop. handle input, do a physics tick, send results to renderer
	while( !shouldExit ) {
		// take a nap first, don't want to work too hard
		usleep(1000)
		
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
							} else if interfaceMode == .workshop {
 								print("\(String(cString: getStringForButton(event.cbutton.button)!)) pressed")
								if		event.cbutton.button == SDL_CONTROLLER_BUTTON_Y.rawValue {
									sens *= sqrt(10.0)
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_X.rawValue {
									sens *= 1.0 / sqrt(10.0)
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_B.rawValue {
									if(partsPickerIsOpen) {
										pickedPart = CompositeCod.unit()
									} else {
										composites[curcom].b[curbox] = BoxoidCod.unit() // todo: undo
									}
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_A.rawValue {
									if(partsPickerIsOpen) {
										composites[curcom].b.append(contentsOf: pickedPart.b)
									} else {
										composites[curcom].b.append(composites[curcom].b[curbox])
									}
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER.rawValue {
									faceIndex = (faceIndex + 1 % 6)// select next face/previous boxoid
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER.rawValue {
									// open/close parts picker
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP.rawValue {
									dpad = .UP
									// move boxoid
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN.rawValue {
									dpad = .DOWN
									// adjust curvature
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT.rawValue {
									dpad = .LEFT
									// elongate
								} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT.rawValue {
									dpad = .RIGHT
									// select boxoid
								}
							}
						case SDL_CONTROLLERBUTTONUP.rawValue:
							if event.cbutton.button == SDL_CONTROLLER_BUTTON_START.rawValue {
								shouldExit = true
							} else if event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK.rawValue {
								// cycle through game modes
								let availableModes: [InterfaceMode] = [.workshop, .physicsSim]
/* not everything has been implemented yet
enum InterfaceMode {
	case mainMenu
	case physicsSim
	case workshop
	case combat
	case travel
	case mining
	case rts
}*/
								interfaceMode = availableModes[interfaceModeIndex++ % availableModes.count]
								rcsIsEnabled = !rcsIsEnabled
							}
						case SDL_CONTROLLERAXISMOTION.rawValue:
							if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTX.rawValue) {
								thrustVector.x = sens * Double(event.caxis.value) / TMAX
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTY.rawValue) {
								thrustVector.y = sens * Double(event.caxis.value) / TMAX
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERLEFT.rawValue) {
								thrustVector.z = sens * -Double(event.caxis.value) / TMAX
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERRIGHT.rawValue) {
								thrustVector.z = sens * Double(event.caxis.value) / TMAX
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTX.rawValue) {
								cameraSpherical.theta = sens * -Double(event.caxis.value) / TMAX
							} else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTY.rawValue) {
								cameraSpherical.phi = sens * Double(event.caxis.value) / TMAX
							}
						// FIXME use glfw
/*
// Callback functions for key and mouse button events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        dt *= 10.0;
    // And so on for other keys...
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        // Handle left mouse button press...
}

// In your initialization code:
glfwSetKeyCallback(window, key_callback);
glfwSetMouseButtonCallback(window, mouse_button_callback);
*/
						case SDL_KEYDOWN.rawValue:
							let key = event.key.keysym.sym
							if interfaceMode == .physicsSim {
								if key == SDLK_w.rawValue { // Y button
									dt *= 10.0
								} else if key == SDLK_a.rawValue { // X button
									dt *= 0.1 
								} else if key == SDLK_s.rawValue { // B button
									buttonPresses += 1;
								} else if key == SDLK_d.rawValue { // A button
									buttonPresses -= 1;
								}
							} else if interfaceMode == .workshop {
								// put stuff here
							}
						case SDL_KEYUP.rawValue:
							let key = event.key.keysym.sym
							if key == SDLK_ESCAPE.rawValue { // START button
								shouldExit = true
							} else if key == SDLK_BACKSPACE.rawValue { // BACK button
								rcsIsEnabled = !rcsIsEnabled
							}
						case SDL_MOUSEMOTION.rawValue:
							let mouseX = Double(event.motion.x)
							thrustVector.x = sens * mouseX / 1920.0 // adjust for your window width
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
			for (i, composite) in composites.enumerated() {
				var ccod = composite
				ccod.position -= camera.position
				let c = toC(ccod)
				objrefs.append(submitComposite(c))
				print("new object ", objrefs[i])
			}
		}
		if interfaceMode == .physicsSim {
			if(rcsIsEnabled && interfaceMode == .physicsSim) {
				actions = [Action(object: player1, force: thrustVector * 10000.0 / dt, torque: Vector(cameraSpherical.phi, 0, cameraSpherical.theta) * -1000.0)]
			} else {
				actions = [Action(object: player1, force: thrustVector * 10000.0 / dt, torque: Vector(0, 0, 0))]
			}
			tick(actions: actions, movingObjects: &allTheThings, t: t, dt: dt)
			t += dt
			composites[curcom].orientation = player1.orientation
			composites[curcom].position = player1.position
		} else if interfaceMode == .workshop {
			// Elongaaaate
			if(thrustVector.length > 0.05) {
				composites[curcom].b[curbox].elongate(thrustVector * 0.01)
				updateComposite(objrefs[curcom], toC(composites[curcom]))
				player1.radius = player1.radius * 0.95 + 0.05 * composites[curcom].bbox.halfsize
			}
		}


		// camera and rendering
		var renderMisc = render_misc()
		renderMisc.buttonPresses = abs(buttonPresses);
		renderMisc.materials = materialsArray
		if interfaceMode == .physicsSim {
			let cameraTarget = player1
			let nearestCelestial = moon
			let relativeVelocity = cameraTarget.velocity - nearestCelestial.velocity
			var prograde = relativeVelocity
			if(relativeVelocity.lengthSquared == 0) {
				camera.position = cameraTarget.position + Vector(10, 10, -4.5 * cameraTarget.radius)
				prograde = cameraTarget.orientation * Vector(0, 0, 1)
			} else {
				prograde = relativeVelocity.normalized()
				camera.position = cameraTarget.position + relativeVelocity.normalized() * -4.5 * cameraTarget.radius
			}
			var camFwd = (cameraTarget.position - camera.position).normalized()
			var upVec = (cameraTarget.position - nearestCelestial.position).normalized()
			let pitchAxis = camFwd.cross(upVec).normalized()
			let pitchQuat = Quaternion(axis: pitchAxis, angle: 2.0 * cameraSpherical.phi * (rcsIsEnabled ? 0.0 : 1.0))
			let yawQuat = Quaternion(axis: upVec, angle: 2.0 * cameraSpherical.theta * (rcsIsEnabled ? 0.0 : 1.0))
			camFwd = yawQuat * pitchQuat * camFwd
			//camFwd = yawQuat * pitchQuat * cameraTarget.orientation * camFwd
			//upVec = pitchQuat * cameraTarget.orientation * upVec
			upVec = pitchQuat * upVec
			renderMisc.camForward = (Float(camFwd.x), Float(camFwd.y), Float(camFwd.z))
			renderMisc.camUp = (Float(upVec.x), Float(upVec.y), Float(upVec.z))
		} else if interfaceMode == .workshop {
			renderMisc.camUp = (0.1, 0.9899, -0.1)
			renderMisc.camForward = (-0.4, -0.4, -0.82462)
		}
		// camera is at 0,0,0 to make it easy for the renderer
		renderMisc.camPosition = (0, 0, 0)
		for (index, object) in lights.enumerated() {
			if(index >= MAX_LIGHTS){
				print("warning: MAX_LIGHTS exceeded")
				break
			}
			withUnsafeMutablePointer(to:&renderMisc.lights) { lights_ptr in
				lights_ptr[index].position =
					(Float(object.position.x - camera.position.x),
					 Float(object.position.y - camera.position.y),
					 Float(object.position.z - camera.position.z))
				// hardcoded values for the sun, improve later
				lights_ptr[index].color = (4.38e24, 4.38e24, 4.18e24)
			}
		}
	

		// we have to sort the things before we send them to the renderer, otherwise transparency breaks.
		allTheThings.sort(by: { ($0.position - camera.position).lengthSquared > ($1.position - camera.position).lengthSquared })
		var trajectory: [Vector] = []
		//var trajectory = extrapolateTrajectory(subject: cameraTarget, relativeTo: nearestCelestial, otherObjects: allTheThings, t: t, dt: 10.0, iterations: 500)
		trajectory.sort(by: { ($0 - camera.position).lengthSquared > ($1 - camera.position).lengthSquared })
		//print("cameraTarget: \(cameraTarget.position)\ntrajectory: \(trajectory)\n")
		//let sphereArray = UnsafeMutablePointer<sphere>.allocate(capacity: allTheThings.count)
		let sphereArray = UnsafeMutablePointer<Sphere>.allocate(capacity: allTheThings.count + trajectory.count)
		for (index, object) in allTheThings.enumerated() {
			// the mapping from object.id to material_idx should not be 1:1 in the future but it's good enough for now
			// center everything on the camera before converting to float to avoid float precision issues when rendering
			sphereArray[index + trajectory.count] = Sphere(position: (Float(object.position.x - camera.position.x),
										Float(object.position.y - camera.position.y),
										Float(object.position.z - camera.position.z)),
										radius: Float(object.radius),
										material_idx: (Int32(object.id)))
		}
		for (index, object) in trajectory.enumerated() {
			// center everything on the camera before converting to float to avoid float precision issues when rendering
			let position = Vector(object.x - camera.position.x, object.y - camera.position.y, object.z - camera.position.z)
			sphereArray[index] = Sphere(position: (Float(position.x),
										Float(position.y),
										Float(position.z)),
										radius: Float(player1.radius * pow(position.length * 0.01, 0.9)),
										material_idx: 6)
		}
		let compositeArray = UnsafeMutablePointer<Objref>.allocate(capacity: objrefs.count)
		for (index, object) in objrefs.enumerated() {
			var position = composites[object.id].position - camera.position
			compositeArray[index] = Objref(orientation: object.orientation, position: position.float, id: object.id, type: object.type)
		}

		//print("sending \(objrefs.count) composites to rendering \(compositeArray[0]) ", player1.position, player1.radius)
		//print("camera position: \(camera.position)")
		render(compositeArray, objrefs.count, sphereArray, allTheThings.count + trajectory.count, renderMisc)

		sphereArray.deallocate()
	}
	// end main game loop

	saveGameState()

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
*/
