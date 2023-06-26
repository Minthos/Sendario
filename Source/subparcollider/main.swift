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
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 1.989e30, radius: 696.34e6, frictionCoefficient: 0.8)
var mercury = SphericalCow(id: 1,
                         position: Vector(x: 0, y: -57.91e9, z: 0),
                         velocity: Vector(x: 0, y: 0, z: 47.87e3),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 3.285e23, radius: 2.44e6, frictionCoefficient: 0.8)
var venus = SphericalCow(id: 2,
                         position: Vector(x: 0, y: 108.2e9, z: 0),
                         velocity: Vector(x: 00, y: 0, z: 35.02e3),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 4.867e24, radius: 6.0518e6, frictionCoefficient: 0.8)
var earth = SphericalCow(id: 3,
                         position: Vector(x: 0, y: 0, z: -149.6e9),
                         velocity: Vector(x: 29.78e3, y: 0, z: 0),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 5.972e24, radius: 6.371e6, frictionCoefficient: 0.0001)
var moon = SphericalCow(id: 4,
                         position: Vector(x: earth.position.x, y: earth.position.y - 384e5, z: earth.position.z),
                         velocity: Vector(x: earth.velocity.x, y: earth.velocity.y, z: earth.velocity.z + 3.66e3),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 7.342e22, radius: 1.7371e6, frictionCoefficient: 0.01)
var camera = SphericalCow(id: -1,
                          position: Vector(x: earth.position.x, y: earth.position.y - earth.radius * 2.0, z: earth.position.z - earth.radius * 8.0),
                          velocity: earth.velocity,
                          orientation: Vector(x: 0, y: 0, z: 0),
                          spin: Vector(x: 0, y: 0, z: 0),
                          mass: 0, radius: 0, frictionCoefficient: 0.0)

var lights = [sun]
var allTheThings = [sun, mercury, venus, earth, moon]
let actions: [Action] = []

let totalTime = 1e9
var dt = 1.0
//let totalTime = 0.0001
//var dt = 0.00001
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
    let materialsArray = UnsafeMutablePointer<material>.allocate(capacity: allTheThings.count)
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

    startRenderer()
    while t < totalTime {
        tick(actions: actions, movingObjects: &allTheThings, t: t, dt: dt)
        t += dt
        //usleep(1000)

        var renderMisc = render_misc()
        renderMisc.materials = materialsArray

        if(false){
            camera.position = earth.position
            camera.position.x += earth.radius * 16.25
            camera.position.z += earth.radius * 0.5
            camera.orientation = (earth.position - camera.position).normalized()
        } else {
            camera.position = moon.position + (moon.position - earth.position).normalized() * 10.0 * moon.radius
            camera.position.x += moon.radius * 2.0
            camera.position.y += moon.radius * 2.0
            camera.position.z += moon.radius * 2.0
            camera.orientation = (moon.position - camera.position).normalized()
        }
        renderMisc.camDirection = (Float(camera.orientation.x), Float(camera.orientation.y), Float(camera.orientation.z))
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
        allTheThings.sort(by: { ($0.position - camera.position).length > ($1.position - camera.position).length })
        let sphereArray = UnsafeMutablePointer<sphere>.allocate(capacity: allTheThings.count)
        for (index, object) in allTheThings.enumerated() {
            // center everything on the camera before converting to float to avoid float precision issues when rendering
            sphereArray[index] = sphere(x: Float(object.position.x - camera.position.x),
                                        y: Float(object.position.y - camera.position.y),
                                        z: Float(object.position.z - camera.position.z),
                                        radius: Float(object.radius),
                                        material_idx: (UInt64(object.id)))
        }

        render(sphereArray, allTheThings.count, renderMisc)
        sphereArray.deallocate()
    }

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

