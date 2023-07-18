import Foundation

let G: Double = 6.67430e-11

struct Vector: Codable {
    var x: Double
    var y: Double
    var z: Double
   
    init(_ coords: (Float, Float, Float)) {
		self.x = Double(coords.0)
		self.y = Double(coords.1)
		self.z = Double(coords.2)
	}

    init(_ coords: (Double, Double, Double)) {
		self.x = coords.0
		self.y = coords.1
		self.z = coords.2
	}

    init(x: Double, y: Double, z: Double) {
        self.x = x
        self.y = y
        self.z = z
        sanityCheck()
    }

    var spherical: Spherical {
        let rho = sqrt(x * x + y * y + z * z)
        let theta = atan2(y, x)
        let cosPhi = z / rho
        let phi = acos(cosPhi)
        return Spherical(rho, theta, phi)
    }

	var float: (Float, Float, Float) {
		return (Float(x), Float(y), Float(z))
	}

#if DEBUG
    func sanityCheck() {
        assert( !x.isNaN, "x is NaN")
        assert( !y.isNaN, "y is NaN")
        assert( !z.isNaN, "z is NaN")
    }
#else
    func sanityCheck() {}
#endif

    func format(_ decimalPlaces: Int) -> String {
        return "(\(String(format: "%.\(decimalPlaces)f", x)), \(String(format: "%.\(decimalPlaces)f", y)), \(String(format: "%.\(decimalPlaces)f", z)))"
    }

    var lengthSquared: Double {
        sanityCheck()
        return x * x + y * y + z * z
    }

    var length: Double {
        sanityCheck()
        return sqrt(x * x + y * y + z * z)
    }
    
    func dot(_ other: Vector) -> Double {
        sanityCheck()
        return x * other.x + y * other.y + z * other.z
    }

    func normalized() -> Vector {
        sanityCheck()
        let length = self.length
        if length == 0 {
            return self
        }
        return self / length
    }

    static func +(lhs: Vector, rhs: Vector) -> Vector {
        lhs.sanityCheck()
        rhs.sanityCheck()
        return Vector(x: lhs.x + rhs.x, y: lhs.y + rhs.y, z: lhs.z + rhs.z)
    }
    
    static func -(lhs: Vector, rhs: Vector) -> Vector {
        lhs.sanityCheck()
        rhs.sanityCheck()
        return Vector(x: lhs.x - rhs.x, y: lhs.y - rhs.y, z: lhs.z - rhs.z)
    }
    
    static func *(lhs: Double, rhs: Vector) -> Vector {
        return rhs * lhs
    }

    static func *(lhs: Vector, rhs: Double) -> Vector {
        assert(!rhs.isNaN)
        lhs.sanityCheck()
        return Vector(x: lhs.x * rhs, y: lhs.y * rhs, z: lhs.z * rhs)
    }
    
    static func /(lhs: Double, rhs: Vector) -> Vector {
        return rhs * lhs
    }

    static func /(lhs: Vector, rhs: Double) -> Vector {
        lhs.sanityCheck()
        assert(rhs != 0)
        assert(!rhs.isNaN)
        return Vector(x: lhs.x / rhs, y: lhs.y / rhs, z: lhs.z / rhs)
    }
    
    static func +=(lhs: inout Vector, rhs: Vector) {
        lhs.sanityCheck()
        rhs.sanityCheck()
        lhs.x += rhs.x
        lhs.y += rhs.y
        lhs.z += rhs.z
    }
    
    static func -=(lhs: inout Vector, rhs: Vector) {
        lhs.sanityCheck()
        rhs.sanityCheck()
        lhs.x -= rhs.x
        lhs.y -= rhs.y
        lhs.z -= rhs.z
    }
    
    static func *=(lhs: inout Vector, rhs: Double) {
        lhs.sanityCheck()
        assert(!rhs.isNaN)
        lhs.x *= rhs
        lhs.y *= rhs
        lhs.z *= rhs
    }
    
    static func /=(lhs: inout Vector, rhs: Double) {
        let inv = 1.0 / rhs
        lhs.x *= inv
        lhs.y *= inv
        lhs.z *= inv
        assert(rhs != 0)
        assert(!rhs.isNaN)
        lhs.sanityCheck()
    }

    func cross(_ other: Vector) -> Vector {
        sanityCheck()
        other.sanityCheck()
        return Vector(
            x: y * other.z - z * other.y,
            y: z * other.x - x * other.z,
            z: x * other.y - y * other.x
        )
    }

    static prefix func - (vector: Vector) -> Vector {
        vector.sanityCheck()
        return Vector(x: -vector.x, y: -vector.y, z: -vector.z)
    }
}

struct Quaternion: Codable {
    var w: Double
    var x: Double
    var y: Double
    var z: Double
    
    init(w: Double, v: Vector) {
        self.w = w
        x = v.x
        y = v.y
        z = v.z
        sanityCheck()
    }

    init(w: Double, x: Double, y: Double, z: Double) {
        self.w = w
        self.x = x
        self.y = y
        self.z = z
        sanityCheck()
    }

    init(axis: Vector, angle: Double) {
        let vn = axis.normalized()
        let halfAngle = angle * 0.5
	let sinAngle = sin(halfAngle);
	x = (vn.x * sinAngle);
	y = (vn.y * sinAngle);
	z = (vn.z * sinAngle);
	w = cos(angle);
        sanityCheck()
    }

    init(axis: (Double, Double, Double), angle: Double) {
        let halfAngle = angle * 0.5
        let s = sin(halfAngle)
        w = cos(halfAngle)
        x = axis.0 * s
        y = axis.1 * s
        z = axis.2 * s
        sanityCheck()
    }
/*
    init(pitch: Double, yaw: Double, roll: Double)
    {
	// Basically we create 3 Quaternions, one for pitch, one for yaw, one for roll
	// and multiply those together.
	// the calculation below does the same, just shorter

	let p = pitch * Double.pi;
	let y = yaw * Double.pi;
	let r = roll * Double.pi;
	//let p = pitch * PIOVER180 / 2.0;
	//let y = yaw * PIOVER180 / 2.0;
	//let r = roll * PIOVER180 / 2.0;

	let sinp = sin(p);
	let siny = sin(y);
	let sinr = sin(r);
	let cosp = cos(p);
	let cosy = cos(y);
	let cosr = cos(r);
        var tmp = Quaternion(x: sinr * cosp * cosy - cosr * sinp * siny,
	                     y: cosr * sinp * cosy + sinr * cosp * siny,
                             z: cosr * cosp * siny - sinr * sinp * cosy,
	                     w: cosr * cosp * cosy + sinr * sinp * siny)

        let len_inv = 1.0 / tmp.length()
        w = tmp.w * len_inv
        x = tmp.x * len_inv
        y = tmp.y * len_inv
        z = tmp.z * len_inv
    }
*/
#if DEBUG
    func sanityCheck() {
        assert( !w.isNaN, "w is NaN")
        assert( !x.isNaN, "x is NaN")
        assert( !y.isNaN, "y is NaN")
        assert( !z.isNaN, "z is NaN")
    }
#else
    func sanityCheck() {}
#endif

	var float: (Float, Float, Float, Float) {
		return (Float(w), Float(x), Float(y), Float(z))
	}

    func getAxisAngle() -> (Vector, Double) {
		let scale = 1.0 / sqrt(x * x + y * y + z * z)
        let axis = Vector(x: x * scale, y: y * scale, z: z * scale)
		return (axis, acos(w) * 2.0)
    }

    func length() -> Double {
        return sqrt(w*w + x*x + y*y + z*z)
    }

    func normalized() -> Quaternion {
        let len_inv = 1.0 / length()
        return Quaternion(w: w * len_inv, x: x * len_inv, y: y * len_inv, z: z * len_inv)
    }

    // if self is unit length, use conjugate instead
    func inverse() -> Quaternion {
        let f = 1.0 / -(w*w + x*x + y*y + z*z)
        return Quaternion(w: w*(-f), x: x*f, y: y*f, z: z*f)
    }
    
    func conjugate() -> Quaternion {
        return Quaternion(w: w, x: -x, y: -y, z: -z)
    }
    
    static func +(left: Quaternion, right: Quaternion) -> Quaternion {
        return Quaternion(w: left.w + right.w, x: left.x + right.x, y: left.y + right.y, z: left.z + right.z)
    }
    
    static func -(left: Quaternion, right: Quaternion) -> Quaternion {
        return Quaternion(w: left.w - right.w, x: left.x - right.x, y: left.y - right.y, z: left.z - right.z)
    }
    
    static func *(left: Quaternion, right: Quaternion) -> Quaternion {
        let w = left.w*right.w - left.x*right.x - left.y*right.y - left.z*right.z
        let x = left.w*right.x + left.x*right.w + left.y*right.z - left.z*right.y
        let y = left.w*right.y - left.x*right.z + left.y*right.w + left.z*right.x
        let z = left.w*right.z + left.x*right.y - left.y*right.x + left.z*right.w
        return Quaternion(w: w, x: x, y: y, z: z)
    }

    static func *(left: Quaternion, right: Vector) -> Vector {
	let vn = right.normalized()
        let vecQuat = Quaternion(w: 0, x: vn.x, y: vn.y, z: vn.z)
	let resQuat = left * vecQuat * left.conjugate()
	return Vector(x: resQuat.x, y: resQuat.y, z: resQuat.z)
    }
}

// assuming Earthlike conditions for now
func atmosphericProperties(altitude: Double) -> (density: Double, pressure: Double, viscosity: Double) {
    let seaLevelTemperature: Double = 288.15
    let temperatureLapseRate: Double = -0.0065
    let seaLevelPressure: Double = 101325.0
    let gasConstant: Double = 287.05
    let gravityConstant: Double = 9.81
    let airMolarMass: Double = 28.97e-3
    let boltzmannConstant: Double = 1.38064852e-23

    let temperature = seaLevelTemperature + temperatureLapseRate * altitude
    let pressure = seaLevelPressure * pow(1 + temperatureLapseRate * altitude / seaLevelTemperature, -gravityConstant * airMolarMass / (gasConstant * temperatureLapseRate))
    let density = pressure / (gasConstant * temperature)
    let viscosity = (boltzmannConstant * temperature) / (sqrt(2) * Double.pi * pow(airMolarMass, 2))

    return (density, pressure, viscosity)
}


struct Spherical: Codable {
    var rho: Double
    var theta: Double
    var phi: Double

    init(_ rho: Double, _ theta: Double, _ phi: Double) {
        self.rho = rho
        self.theta = theta
        self.phi = phi
    }

    var cartesian: Vector {
        let sinPhi = sin(phi)
        let x = rho * sinPhi * cos(theta)
        let y = rho * sinPhi * sin(theta)
        let z = rho * cos(phi)
        return Vector(x: x, y: y, z: z)
    }
   
/*    var pretty: String {
        return "(rho: \(rho.pretty), theta: \(theta.pretty), phi: \(phi.pretty))"
    }
    
    var scientific: String {
        return "SphericalPoint(rho: \(rho.scientific), theta: \(theta.scientific), phi: \(phi.scientific))"
    }*/
}

class Celestial: Codable {
    var moo: SphericalCow
    var perigee: Spherical
    var apogee: Spherical
    var name: String
    var gravitationalParameter: Double
    var atmosphereHeight: Double?
    var atmosphereComposition: [String: Double]?
    var meanTemperature: Double?
    var albedo: Double?
    var rotationPeriod: Double?
    
    init(moo: SphericalCow, perigee: Spherical, apogee: Spherical, name: String) {
        self.moo = moo
        self.perigee = perigee
        self.apogee = apogee
        self.name = name
        self.gravitationalParameter = 6.67430e-11 * moo.mass // Standard gravitational parameter
    }
    
    func escapeVelocity(at distance: Double) -> Double {
        return sqrt(2 * gravitationalParameter / distance)
    }
    
    func orbitVelocity(at distance: Double) -> Double {
        return sqrt(gravitationalParameter / distance)
    }
    
    func period(forOrbitRadius radius: Double) -> Double {
        return 2 * Double.pi * sqrt(pow(radius, 3) / gravitationalParameter)
    }
    
    func surfaceGravity(at distance: Double) -> Double {
        return gravitationalParameter / pow(distance, 2)
    }
    
    func surfaceAcceleration(at distance: Double) -> Vector {
        // This method can take into account the rotation and other physical factors of the celestial body
        // You can implement it based on your specific requirements
        // Here's an example implementation that ignores rotation:
        let gravity = surfaceGravity(at: distance)
        let surfaceNormal = Vector(x: 0, y: 0, z: -1).normalized() // Assumes the body is a sphere with its center at the origin
        return surfaceNormal * gravity
    }
    
    func surfaceTemperature(at distance: Double) -> Double {
        // This method can take into account the distance of the body from the sun (if applicable),
        // its albedo, and other physical factors
        // You can implement it based on your specific requirements
        return meanTemperature ?? 0 // Placeholder implementation
    }
    
    func atmospherePressure(at distance: Double) -> Double? {
        guard let atmosphereHeight = atmosphereHeight else {
            return nil
        }
        if distance > atmosphereHeight {
            return 0
        }
        let altitude = atmosphereHeight - distance
        // This method can take into account the composition of the atmosphere and other physical factors
        // You can implement it based on your specific requirements
        return 101325 * pow((288.15 - 0.0065 * altitude) / 288.15, 5.25588) // Placeholder implementation (based on the International Standard Atmosphere)
    }
}

// 1. create a celestial class with perigee and apogee and compare stored values to measured values to detect drift
// 2. upgrade gravity to handle planets and moons
// 3. upgrade celestial class with reentry heat and drag
// 4. upgrade collisions to be not instantaneous and handle multiple bodies colliding. 1 ms default timestep
// shorter timestep as needed during high acceleration such as collisions
// longer timestep for planet trajectories, they rarely change anyway. Only the position changes every ms.
class SphericalCow: Codable {
    let id: Int64
    var position: Vector
    var relativeTo: Celestial?
    var velocity: Vector
    var orientation: Quaternion
    var spin: Vector
    var accumulatedForce: Vector
    var accumulatedTorque: Vector
    var prevForce: Vector
    var prevTorque: Vector
    let mass: Double
    let radius: Double
    let momentOfInertia: Double // should be 3x3 matrix (inertia tensor)
    let frictionCoefficient: Double
    var inertia: Double { return (2 / 5) * mass * pow(radius, 2) }
    var density: Double { return mass / (4 * Double.pi * pow(radius, 3) / 3) }
    var heat: Double = 0

    init(id: Int64, position: Vector, velocity: Vector, orientation: Quaternion, spin: Vector, mass: Double, radius: Double, frictionCoefficient: Double = 0.1) {
        self.id = id
        self.position = position
        self.velocity = velocity
        self.orientation = orientation
        self.spin = spin
        self.accumulatedForce = Vector(x: 0, y: 0, z: 0)
        self.accumulatedTorque = Vector(x: 0, y: 0, z: 0)
        self.prevForce = Vector(x: 0, y: 0, z: 0)
        self.prevTorque = Vector(x: 0, y: 0, z: 0)
        self.mass = mass
        self.radius = radius
        self.momentOfInertia = 2 * mass * radius * radius / 5
        self.frictionCoefficient = frictionCoefficient
    }

    func copy() -> SphericalCow {
        return SphericalCow(id: id, position: position, velocity: velocity, orientation: orientation, spin: spin, mass: mass, radius: radius, frictionCoefficient: frictionCoefficient)
    }

    func applyForce(force: Vector, dt: Double) {
        self.accumulatedForce += force
    }

    func applyTorque(torque: Vector, dt: Double) {
        self.accumulatedTorque += torque
    }

    func integrateForce(dt: Double) {
        let new_pos = position + velocity * dt + (prevForce / mass) * (dt * dt * 0.5)
        let sum_accel = (prevForce + accumulatedForce) / mass
        let new_vel = velocity + (sum_accel)*(dt*0.5)
        position = new_pos
        velocity = new_vel
        prevForce = accumulatedForce
        accumulatedForce = Vector(x: 0, y: 0, z: 0)
    }

// oemga should probably be rotated 90 degrees or something
    func integrateTorque(dt: Double) {
        let omega = (spin * dt) + ((prevTorque / momentOfInertia) * (dt * dt * 0.5))
        let angle = omega.length
        let rotation = Quaternion(axis: omega, angle: angle)
        if(omega.x != 0 || omega.y != 0 || omega.z != 0) {
            orientation = (rotation * orientation).normalized()
    //        print("spin: \(spin) omega: \(omega)")
        }
        let sum_rotAccel = (prevTorque + accumulatedTorque) / momentOfInertia
        let new_spin = spin + (sum_rotAccel)*(dt*0.5)
        spin = new_spin
        prevTorque = accumulatedTorque
        accumulatedTorque = Vector(x: 0, y: 0, z: 0)
    }

}

func calculateGravities(subject: SphericalCow, objects: [SphericalCow]) -> Vector {
    var gravity = Vector(x: 0, y: 0, z: 0)
    for object in objects {
        if(subject !== object) {
            let deltaPosition = object.position - subject.position
            let distance = deltaPosition.length
            var mass = object.mass
            if(distance == 0) {
                continue
            } else if(distance < object.radius) {
                let volumeEnclosed = (4.0 / 3.0) * Double.pi * pow(distance, 3)
                mass = object.density * volumeEnclosed
            }
            let forceMagnitude = (G * mass * subject.mass) / pow(distance, 2)
            gravity += deltaPosition.normalized() * forceMagnitude
        }
    }
    return gravity
}

func extrapolateTrajectory(subject: SphericalCow, relativeTo: SphericalCow, otherObjects: [SphericalCow], t: Double, dt: Double, iterations: Int) -> [Vector] {
    let s = subject.copy()
    let other = relativeTo.copy()
    var c: [SphericalCow] = [s, other]
    for item in otherObjects {
        if(item !== subject && item !== relativeTo) {
            c.append(item.copy())
        }
    }
    let offset = other.position
    var result: [Vector] = []
    for i in 0..<iterations {
        tick(actions: [], movingObjects: &c, t: t + dt * Double(i), dt: dt)
        result.append(s.position + offset - other.position)
    }
    return result
}

func tick(actions: [Action], movingObjects: inout [SphericalCow], t: Double, dt: Double) {
    for action in actions {
        action.object.applyForce(force: action.force, dt: dt)
        action.object.applyTorque(torque: action.torque, dt: dt)
    }

    for object in movingObjects {
        if(object !== sun && object !== camera){
            object.applyForce(force: calculateGravities(subject: object, objects: movingObjects), dt: dt)

            //let altitude = object.position.z - earth.radius
            //let (density, _, viscosity) = atmosphericProperties(altitude: altitude)
            //let dragCoefficient = 0.47 // Assuming a sphere
            //let area = Double.pi * pow(object.radius, 2)
            //var dragForceMagnitude = 0.5 * density * pow(object.velocity.length, 2) * dragCoefficient * area
            //if(dragForceMagnitude.isNaN) {
            //    dragForceMagnitude = 0
            //}
            //let dragForce = -object.velocity.normalized() * dragForceMagnitude
            //object.applyForce(force: dragForce, dt: dt)

            //let angularDragTorqueMagnitude = 8 * Double.pi * viscosity * pow(object.radius, 3) * object.spin.length / 3
            //let angularDragTorque = -object.spin.normalized() * angularDragTorqueMagnitude
            //object.applyTorque(torque: angularDragTorque, dt: dt)

            //let magnusForce = magnusForce(velocity: object.velocity, spin: object.spin, density: density, viscosity: viscosity, radius: object.radius)
            //object.applyForce(force: magnusForce, dt: dt)
            //print("drag: \(dragForceMagnitude), magnus: \(magnusForce.length), gravity: \(gravityForce.length), total: \((dragForce + magnusForce + gravityForce).length)")
//            object.applyHeat(heat: 
        }
        object.integrateForce(dt: dt)
        object.integrateTorque(dt: dt)
    }


    var collisions: [(Double, SphericalCow, SphericalCow)] = []

    for i in 0..<movingObjects.count {
        for j in i+1..<movingObjects.count {
            let object1 = movingObjects[i]
            let object2 = movingObjects[j]
            let deltaPosition = object2.position - object1.position
            let sumRadii = object1.radius + object2.radius
            let distance = deltaPosition.length - sumRadii
            let deltaVelocity = object2.velocity - object1.velocity
            let closingSpeed = deltaVelocity.dot(deltaPosition.normalized())

            if closingSpeed < 0 && distance < (-1.0 * closingSpeed * dt) {
                let collisionTime = distance / closingSpeed
                collisions.append((collisionTime, object1, object2))
            }
        }
    }

    collisions.sort { $0.0 < $1.0 }
    for (elapsedTime, object1, object2) in collisions {
        // rewind this tick's movement and apply movement up to the time of collision
        object1.position -= object1.velocity * (dt - elapsedTime)
        object2.position -= object2.velocity * (dt - elapsedTime)

        let deltaPosition = object2.position - object1.position
        let deltaVelocity = object2.velocity - object1.velocity
        let relativeVelocity = deltaVelocity.dot(deltaPosition) / deltaPosition.length
        let collisionNormal = deltaPosition.normalized()
        let impulseMagnitude = abs(2 * object1.mass * object2.mass * relativeVelocity / (object1.mass + object2.mass))

        let impulse = collisionNormal * impulseMagnitude
        if relativeVelocity < 0 {
            object1.velocity -= impulse / object1.mass
            object2.velocity += impulse / object2.mass
        }

        // Calculate and apply impulse due to friction
        let frictionCoefficient = (object1.frictionCoefficient * object2.frictionCoefficient)
        let tangentVelocity = deltaVelocity - (collisionNormal * deltaVelocity.dot(collisionNormal))
        let frictionImpulseMagnitude = impulseMagnitude * frictionCoefficient
        let frictionImpulse = tangentVelocity.normalized() * frictionImpulseMagnitude

        object1.velocity -= frictionImpulse / object1.mass
        object2.velocity += frictionImpulse / object2.mass

        // Calculate and apply change in angular velocity due to collision
        let collisionPoint1 = object1.position + (collisionNormal * object1.radius)
        let collisionPoint2 = object2.position - (collisionNormal * object2.radius)
        let r1 = collisionPoint1 - object1.position
        let r2 = collisionPoint2 - object2.position

        let angularImpulse1 = r1.cross(impulse + frictionImpulse) / object1.inertia
        let angularImpulse2 = r2.cross(impulse + frictionImpulse) / object2.inertia

        object1.spin += angularImpulse1
        object2.spin -= angularImpulse2

/*        print("Collision Time:", String(format: "%.2f", t + elapsedTime))
        print("Collision Normal:", collisionNormal.format(2))
        print("Impulse Magnitude:", String(format: "%.2f", impulseMagnitude))
        print("Impulse:", impulse.format(2))
        print("Friction Impulse:", frictionImpulse.format(2))
        print("Angular Impulse 1:", angularImpulse1.format(2))
        print("Angular Impulse 2:", angularImpulse2.format(2))
        print()*/

    }

}


func magnusCoefficient(velocity: Vector, spin: Vector, density: Double, viscosity: Double, radius: Double) -> Double {
    let reynoldsNumber = density * velocity.length * 2 * radius / viscosity
    let spinRatio = spin.length / velocity.length
    let spinReynoldsNumber = reynoldsNumber * spinRatio

    if spinReynoldsNumber <= 0 {
        return 0.0
    }

    if spinReynoldsNumber >= 1e5 {
        return 0.2
    }

    let logTerm = log10(spinReynoldsNumber)
    let a = -4.6 * logTerm + 23.4
    let b = -logTerm + 0.4
    let c = -0.4 * logTerm + 2.5
    let magnusCoefficient = (a * pow(spinRatio, b)) / (1 + c * pow(spinRatio, b))

    return magnusCoefficient
}

func magnusForce(velocity: Vector, spin: Vector, density: Double, viscosity: Double, radius: Double) -> Vector {
    let magnusCoefficient = magnusCoefficient(velocity: velocity, spin: spin, density: density, viscosity: viscosity, radius: radius)
    let area = Double.pi * pow(radius, 2)
    let relativeVelocity = velocity.length
    var magnusForceMagnitude = 0.5 * density * pow(relativeVelocity, 2) * magnusCoefficient * area
    if(magnusForceMagnitude.isNaN) {
        magnusForceMagnitude = 0
    }
let magnusForceDirection = velocity.cross(spin).normalized()
    let magnusForce = magnusForceDirection * magnusForceMagnitude
    if(magnusForceMagnitude > 0) {
//        print("magnus force: \(magnusForce.format(2))")
    }
    return magnusForce
}


/*
// straight from gpt-3.5
func dragHeatEnergy(sphereRadius: Double, velocity: Double, altitude: Double) -> (Double, Double) {
    let density = atmosphericDensity(altitude: altitude)
    let area = Double.pi * pow(sphereRadius, 2)
    let dragCoefficient = 0.47 // Assuming a sphere
    let velocitySquared = pow(velocity, 2)
    
    // Calculate total drag energy
    let dragForceMagnitude = 0.5 * density * velocitySquared * dragCoefficient * area
    let dragEnergy = dragForceMagnitude * velocity * secondsPerStep
    
    // Calculate total heat energy
    let Cp = 1004.5 // Specific heat at constant pressure for air at room temperature
    let T0 = atmosphericTemperature(altitude: altitude)
    let Pr = 0.7 // Prandtl number for air at room temperature
    let k = 0.026 // Thermal conductivity of air at room temperature
    let Re = (density * velocity * 2 * sphereRadius) / 1.846e-5 // Reynolds number for a sphere
    let Nu = (2 + 0.4 * pow(Re, 0.5) + 0.06 * pow(Re, 0.66667)) * pow(Pr, 0.33) // Nusselt number for a sphere
    let h = (Nu * k) / (2 * sphereRadius) // Convective heat transfer coefficient
    let Tf = T0 + (dragForceMagnitude / (sphereMass * Cp)) * (1 + 0.5 * (gamma - 1) * MachNumberSquared) // Temperature rise due to friction heating
    let heatFlux = h * (Tf - T0) // Heat flux from the sphere surface
    let heatEnergy = heatFlux * area * secondsPerStep
    return (dragEnergy, heatEnergy)
}

// NASA Ames according to gpt-3.5 -- simplified
func capsuleReentryHeat(radius: Double, mass: Double, velocity: Double, time: Double, altitude: Double) -> (Double, Double) {
    let area = 4 * Double.pi * pow(radius, 2)
    let density = atmosphericDensity(altitude: altitude)
    let heatTransferCoefficient = 100 // W/m^2*K, approximate value for convective heat transfer coefficient
    
    let q = 1.83 * pow(density, 0.67) * pow(velocity, 3.07) * (area / mass) * pow((1 - 0.12 * exp(-0.2 * time)), 2/3)
    let heat = q * heatTransferCoefficient * area * secondsPerStep

    return (q, heat)
}*/
