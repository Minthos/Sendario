// Subpar Collider.
// It's a ahem "physics" "engine"
// It calculates collisions and stuff.

import Foundation
import Glibc

struct Action {
    let object: SphericalCow
    let force: Vector
    let torque: Vector
}

var sun = SphericalCow(id: 0,
                         position: Vector(x: 0, y: 0, z: 0),
                         velocity: Vector(x: 0, y: 0, z: 0),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 1.989e30, radius: 696.34e6, frictionCoefficient: 0.8)
var mercury = SphericalCow(id: 1,
                         position: Vector(x: 0, y: -57.91e9, z: 0),
                         velocity: Vector(x: 0, y: 0, z: 47.87e3),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 3.285e23, radius: 2.44e6, frictionCoefficient: 0.8)
var venus = SphericalCow(id: 2,
                         position: Vector(x: 0, y: 108.2e9, z: 0),
                         velocity: Vector(x: 00, y: 0, z: 35.02e3),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 4.867e24, radius: 6.0518e6, frictionCoefficient: 0.8)
var earth = SphericalCow(id: 3,
                         position: Vector(x: 0, y: 0, z: -149.6e9),
                         velocity: Vector(x: 0, y: 29.78e3, z: 0),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 5.972e24, radius: 6.371e6, frictionCoefficient: 0.0001)
var moon = SphericalCow(id: 4,
                         position: Vector(x: earth.position.x, y: earth.position.y - 384e6, z: earth.position.z),
                         velocity: Vector(x: earth.velocity.x, y: earth.velocity.y, z: earth.velocity.z + 1.022e3),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 7.342e22, radius: 1.7371e6, frictionCoefficient: 0.8)
var player1 = SphericalCow(id: 5,
                         position: Vector(x: moon.position.x, y: moon.position.y, z: moon.position.z + moon.radius + 200000.1),
                         velocity: Vector(x: moon.velocity.x, y: moon.velocity.y + 1600.0, z: moon.velocity.z),
                         orientation: Quaternion(w: 0, x: 0, y: 1, z:0),
                         spin: moon.spin,
                         mass: 10e3, radius:4.0, frictionCoefficient: 0.5)
var camera = SphericalCow(id: -1,
                          position: Vector(x: earth.position.x, y: earth.position.y - earth.radius * 2.0, z: earth.position.z - earth.radius * 8.0),
                          velocity: earth.velocity,
                          orientation: Quaternion(w: 0, x: 0, y: 1, z: 0),
                          spin: Vector(x: 0, y: 0, z: 0),
                          mass: 0, radius: 0, frictionCoefficient: 0.0)

var lights = [sun]
var allTheThings = [sun, mercury, venus, earth, moon, player1]
var actions: [Action] = []
var buttonPresses: Int32 = 0

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
    case toggleAutopilot // left stick button: toggle autopilot (if target locked, match velocity with locked target)
    case toggleCameraDefault // right stick button: set camera direction to prograde / set camera direction to ship's forward direction
    case forwardThrust // right throttle
    case backwardThrust // left throttle
    case thrustVector // left stick
    case cameraVector // right stick
}

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

    startRenderer()
    SDL_Init(SDL_INIT_GAMECONTROLLER)
    var controller: OpaquePointer? = nil
    var shouldExit = false
    var rcsIsEnabled = false
    //let worldUpVector = Vector(x: 0, y: 1, z:0)
    var thrustVector = Vector(x: 0, y: 0, z: 0)
    var cameraSpherical = Spherical(1, 0, 0)
    
    // main game loop. handle input, do a physics tick, send results to renderer
    while( !shouldExit ) {
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
                            print("\(String(cString: getStringForButton(event.cbutton.button)!)) pressed")
                            if        event.cbutton.button == SDL_CONTROLLER_BUTTON_Y.rawValue {
                                dt *= 10.0
                            } else if event.cbutton.button == SDL_CONTROLLER_BUTTON_X.rawValue {
                                dt *= 0.1
                            } else if event.cbutton.button == SDL_CONTROLLER_BUTTON_B.rawValue {
                                buttonPresses += 1;
                            } else if event.cbutton.button == SDL_CONTROLLER_BUTTON_A.rawValue {
                                buttonPresses -= 1;
                            }
                        case SDL_CONTROLLERBUTTONUP.rawValue:
                            if event.cbutton.button == SDL_CONTROLLER_BUTTON_START.rawValue {
                                shouldExit = true
                            } else if event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK.rawValue {
                                rcsIsEnabled = !rcsIsEnabled
                            }
                        case SDL_CONTROLLERAXISMOTION.rawValue:
                            if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTX.rawValue) {
                                thrustVector.x = Double(event.caxis.value) / TMAX
                            } else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_LEFTY.rawValue) {
                                thrustVector.y = Double(event.caxis.value) / TMAX
                            } else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERLEFT.rawValue) {
                                thrustVector.z = -Double(event.caxis.value) / TMAX
                            } else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_TRIGGERRIGHT.rawValue) {
                                thrustVector.z = Double(event.caxis.value) / TMAX
                            } else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTX.rawValue) {
                                cameraSpherical.theta = -Double(event.caxis.value) / TMAX
                            } else if(event.caxis.axis ==  SDL_CONTROLLER_AXIS_RIGHTY.rawValue) {
                                cameraSpherical.phi = Double(event.caxis.value) / TMAX
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
        if(rcsIsEnabled) {
            actions = [Action(object: player1, force: thrustVector * 10000.0 / dt, torque: Vector(x: cameraSpherical.phi, y: 0, z: cameraSpherical.theta) * -1000.0)]
        } else {
            actions = [Action(object: player1, force: thrustVector * 10000.0 / dt, torque: Vector(x: 0, y: 0, z: 0))]
        }
        tick(actions: actions, movingObjects: &allTheThings, t: t, dt: dt)
        t += dt
        usleep(1000)

        // camera and rendering
        var renderMisc = render_misc()
        renderMisc.materials = materialsArray
        let cameraTarget = player1
        let nearestCelestial = moon
        let relativeVelocity = cameraTarget.velocity - nearestCelestial.velocity
        var prograde = relativeVelocity
        if(relativeVelocity.lengthSquared == 0) {
            camera.position = cameraTarget.position + Vector(x: 0, y: 0, z: -1.5 * cameraTarget.radius)
            prograde = cameraTarget.orientation * Vector(x: 0, y: 0, z: 1)
        } else {
            prograde = relativeVelocity.normalized()
            camera.position = cameraTarget.position + relativeVelocity.normalized() * -1.5 * cameraTarget.radius
        }
        var camFwd = (cameraTarget.position - camera.position).normalized()
        var upVec = (cameraTarget.position - nearestCelestial.position).normalized()
        let pitchAxis = camFwd.cross(upVec).normalized()
        let pitchQuat = Quaternion(axis: pitchAxis, angle: 2.0 * cameraSpherical.phi * (rcsIsEnabled ? 0.0 : 1.0))
        let yawQuat = Quaternion(axis: upVec, angle: 2.0 * cameraSpherical.theta * (rcsIsEnabled ? 0.0 : 1.0))
        camFwd = yawQuat * pitchQuat * cameraTarget.orientation * camFwd
        upVec = pitchQuat * cameraTarget.orientation * upVec
        renderMisc.camForward = (Float(camFwd.x), Float(camFwd.y), Float(camFwd.z))
        renderMisc.camUp = (Float(upVec.x), Float(upVec.y), Float(upVec.z))

        // delete this, it's just to silence a warning
        if(prograde.y == 0){
            print("")
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
            let position = Vector(x: object.x - camera.position.x, y: object.y - camera.position.y, z: object.z - camera.position.z)
            sphereArray[index] = Sphere(position: (Float(position.x),
                                        Float(position.y),
                                        Float(position.z)),
                                        radius: Float(player1.radius * pow(position.length * 0.01, 0.9)),
                                        material_idx: 6)
        }
		renderMisc.buttonPresses = abs(buttonPresses);
        render(sphereArray, allTheThings.count + trajectory.count, renderMisc)
        sphereArray.deallocate()
    }
    // end main game loop

    SDL_Quit()
    stopRenderer()
    materialsArray.deallocate()
}

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

main()

