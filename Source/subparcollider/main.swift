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

var sun = SphericalCow(position: Vector(x: 0, y: 0, z: 0),
                         velocity: Vector(x: 0, y: 0, z: 0),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 1.989e30, radius: 696.34e6, frictionCoefficient: 0.8)
var mercury = SphericalCow(position: Vector(x: 0, y: 0, z: -57.91e6),
                         velocity: Vector(x: 47.87e3, y: 0, z: 0),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 3.285e23, radius: 2.44e6, frictionCoefficient: 0.8)
var venus = SphericalCow(position: Vector(x: 0, y: 0, z: -108.2e6),
                         velocity: Vector(x: 35.02e3, y: 0, z: 0),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 4.867e24, radius: 6.0518e6, frictionCoefficient: 0.8)
var earth = SphericalCow(position: Vector(x: 0, y: 0, z: -149.6e6),
                         velocity: Vector(x: 29.78e3, y: 0, z: 0),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 5.972e24, radius: 6.371e6, frictionCoefficient: 0.8)
var moon = SphericalCow(position: Vector(x: 0, y: 0, z: earth.position.z - 384.4e3),
                         velocity: Vector(x: earth.velocity.x + 1.02e3, y: 0, z: earth.velocity.z),
                         orientation: Vector(x: 0, y: 0, z: 0),
                         spin: Vector(x: 0, y: 0, z: 0),
                         mass: 7.342e22, radius: 1.7371e6, frictionCoefficient: 0.8)
var camera = SphericalCow(position: Vector(x: earth.position.x, y: earth.position.y - earth.radius * 2.0, z: earth.position.z - earth.radius * 8.0),
                          velocity: earth.velocity,
                          orientation: Vector(x: 0, y: 0, z: 0),
                          spin: Vector(x: 0, y: 0, z: 0),
                          mass: 0, radius: 0, frictionCoefficient: 0.0)

var celestials = [sun, mercury, venus, earth, moon]
var allTheThings = [sun, mercury, venus, earth, moon]
let actions: [Action] = []

//let totalTime = 10.0
//var dt = 0.001
let totalTime = 0.0001
var dt = 0.00001
var t = 0.0

func main() {
    startRenderer()
    while t < totalTime {
        tick(actions: actions, movingObjects: &allTheThings, t: t, dt: dt)
        t += dt
        usleep(1000)

        var renderMisc = render_misc()

        camera.position.z = earth.position.z - earth.radius * (8 - t)
        camera.orientation = earth.position - camera.position
        renderMisc.camDirection = (Float(camera.orientation.x), Float(camera.orientation.y), Float(camera.orientation.z))
        renderMisc.camPosition = (Float(camera.position.x), Float(camera.position.y), Float(camera.position.z))

        // we have to sort the things before we send them to the renderer, otherwise transparency breaks.
        // this conveniently puts the camera at the end and earth at the beginning of the array.
        allTheThings.sort(by: { ($0.position - camera.position).length > ($1.position - camera.position).length })

        let sphereArray = UnsafeMutablePointer<sphere>.allocate(capacity: allTheThings.count)
        for (index, object) in allTheThings.enumerated() {
            sphereArray[index] = sphere(x: Float(object.position.x),
                                        y: Float(object.position.y),
                                        z: Float(object.position.z),
                                        radius: Float(object.radius),
                                        material: nil)
        }
        render(sphereArray, allTheThings.count, renderMisc)
        sphereArray.deallocate()
    }

    stopRenderer()
}

main()

