import Foundation

let G: Double = 6.67430e-11

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

protocol Moo {
	var moo: SphericalCow { get }
}

class Entity: Codable, Moo {
	var moo: SphericalCow
	var name: String
	var c: CompositeCod
	
	init(name: String, _ moo: SphericalCow) {
		self.name = name
		self.moo = moo
		self.c = CompositeCod.unit()
	}
}

class Celestial: Codable, Moo {
	var moo: SphericalCow
	var perigee: SphericalVector
	var apogee: SphericalVector
	var name: String
	var gravitationalParameter: Double
	var atmosphereComposition: [String: Double]?
	var meanTemperature: Double?
	var albedo: Double?
	var rotationPeriod: Double?
	
	init(name: String, _ moo: SphericalCow) {
		self.moo = moo
		(self.perigee, self.apogee) = calculatePeriApo(moo)
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
	
	func surfaceTemperature(at distance: Double) -> Double {
		return meanTemperature ?? 0 // Placeholder implementation
	}
	
	func atmospherePressure(at altitude: Double) -> Double? {
		return 101325 * pow((288.15 - 0.0065 * altitude) / 288.15, 5.25588) // based on the International Standard Atmosphere
	}
}

// 3. upgrade celestial class with reentry heat and drag
// 4. upgrade collisions to be not instantaneous and handle multiple bodies colliding. 1 ms default timestep
class SphericalCow: Codable {
	let id: Int64
	var frameOfReference: SphericalCow? = nil
	var position: Vector
	var w: Double
	var velocity: Vector
	var orientation: Quaternion
	var spin: Vector
	var accumulatedForce: Vector
	var accumulatedTorque: Vector
	var prevForce: Vector
	var prevTorque: Vector
	var mass: Double
	var radius: Double
	var momentOfInertia: Double // should be 3x3 matrix (inertia tensor)
	var frictionCoefficient: Double
	var density: Double { return mass / (4 * Double.pi * pow(radius, 3) / 3) }
	var heat: Double = 0

	init(id: Int64, position: Vector, velocity: Vector, orientation: Quaternion, spin: Vector, mass: Double, radius: Double, frictionCoefficient: Double = 0.1) {
		self.id = id
		self.position = position
		self.w = 1.0
		self.velocity = velocity
		self.orientation = orientation
		self.spin = spin
		self.accumulatedForce = Vector(0, 0, 0)
		self.accumulatedTorque = Vector(0, 0, 0)
		self.prevForce = Vector(0, 0, 0)
		self.prevTorque = Vector(0, 0, 0)
		self.mass = mass
		self.radius = radius
		self.momentOfInertia = 2 * mass * radius * radius / 5
		self.frictionCoefficient = frictionCoefficient
	}

	func copy() -> SphericalCow {
		return SphericalCow(id: id, position: position, velocity: velocity, orientation: orientation, spin: spin, mass: mass, radius: radius, frictionCoefficient: frictionCoefficient)
	}

	static func unit() -> SphericalCow {
		return SphericalCow(id: 0, position: Vector(), velocity: Vector(), orientation: Quaternion(), spin: Vector(), mass: 0, radius: 0, frictionCoefficient: 0)
	}

	func updateFrameOfReference(_ new: SphericalCow) {
		let old = self.frameOfReference ?? SphericalCow.unit()
		// let's not do this for now, instead let's keep different star systems in separate coordinate spaces and not mix objects between them.
		//self.position = self.position + old.position - new.position
		//self.velocity = self.velocity + old.velocity - new.velocity
		self.frameOfReference = new
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
		accumulatedForce = Vector(0, 0, 0)
	}

	func integrateTorque(dt: Double) {
		let omega = (spin * dt) + ((prevTorque / momentOfInertia) * (dt * dt * 0.5))
		let angle = omega.length
		let rotation = Quaternion(axis: omega, angle: angle)
		if(omega.x != 0 || omega.y != 0 || omega.z != 0) {
			orientation = (rotation * orientation).normalized()
	//		print("spin: \(spin) omega: \(omega)")
		}
		let sum_rotAccel = (prevTorque + accumulatedTorque) / momentOfInertia
		let new_spin = spin + (sum_rotAccel)*(dt*0.5)
		spin = new_spin
		prevTorque = accumulatedTorque
		accumulatedTorque = Vector(0, 0, 0)
	}

}

func calculatePeriApo(_ moo: SphericalCow) -> (SphericalVector, SphericalVector) {
	return(SphericalVector(0,0,0), SphericalVector(0,0,0))
}

func calculateGravities(subject: SphericalCow, objects: [Celestial]) -> (Vector, SphericalCow) {
	var gravity = Vector(0, 0, 0)
	var strongestForce = 0.0
	var strongestAttractor = subject
	for object in objects {
		if(subject !== object.moo) {
			let deltaPosition = object.moo.position - subject.position
			let distance = deltaPosition.length
			var mass = object.moo.mass
			if(distance == 0) {
				continue
			} else if(distance < object.moo.radius) {
				let volumeEnclosed = (4.0 / 3.0) * Double.pi * pow(distance, 3)
				mass = object.moo.density * volumeEnclosed
			}
			let forceMagnitude = (G * mass * subject.mass) / pow(distance, 2)
			gravity += deltaPosition.normalized() * forceMagnitude
			if forceMagnitude > strongestForce {
				strongestForce = forceMagnitude
				strongestAttractor = object.moo
			}
		}
	}
	return (gravity, strongestAttractor)
}

func extrapolateTrajectory(subject: Moo, relativeTo: Moo, otherObjects: [Celestial], t: Double, dt: Double, iterations: Int) -> [Vector] {
	/*let s = subject.copy()
	let other = relativeTo.copy()
	var c: [Moo] = [s, other]
	for item in otherObjects {
		if(item !== subject && item !== relativeTo) {
			c.append(item.copy())
		}
	}
	let offset = other.moo.position*/
	var result: [Vector] = []
	/*for i in 0..<iterations {
		tick(actions: [], entities: [s], celestials: &other, t: t + dt * Double(i), dt: dt)
		result.append(s.position + offset - other.position)
	}*/
	return result
}

func gravTick(center: SphericalCow, celestials: inout [Celestial], t: Double, dt: Double) {
	for object in celestials {
		if(object.moo !== center){
			let (gravity, nearest) = calculateGravities(subject: object.moo, objects: celestials)
			object.moo.applyForce(force: gravity, dt: dt)
			// these are celestials, they should all have the same frame of reference (their star's)
			//if(nearest !== object.moo.frameOfReference) {
				//object.moo.updateFrameOfReference(nearest)
			//}
		}
		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)
	}
}

// "frame of reference" is zoning in the physics engine.
// should usually be the celestial that causes the strongest local gravity vector.
// frame of reference when local gravity is weak (very far from any celestial) can be an artificial frame of reference
// that is centered on a point in space reasonably close to the entity in consideration. the frame of reference can be
// changed when the distance to the previous one gets above some reasonable limit, say 100 AU or whatever. This can be
// done to avoid
// double precision issues at extremely large distances (interstellar space). This can also be used to simplify the
// orbit calculations of asteroid belts, to create a "fake stable orbit" frame of reference where only the frame moves,
// not the asteroids relative to the frame
// though I guess the physics tick for asteroids can be very slow except when players are interacting with them
// distant celestials can be ignored when local gravity is strong
// star systems don't have to orbit the galactic center, they can just be stationary.
func tick(actions: [Action], entities: inout [Entity], celestials: inout [Celestial], t: Double, dt: Double) {
	for action in actions {
		action.object.applyForce(force: action.force, dt: dt)
		action.object.applyTorque(torque: action.torque, dt: dt)
	}

	for object in entities {
		let (gravity, nearest) = calculateGravities(subject: object.moo, objects: celestials)
		object.moo.applyForce(force: gravity, dt: dt)
		if(nearest !== object.moo.frameOfReference) {
			object.moo.updateFrameOfReference(nearest)
		}
		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)
		//let altitude = object.position.z - earth.radius
		//let (density, _, viscosity) = atmosphericProperties(altitude: altitude)
		//let dragCoefficient = 0.47 // Assuming a sphere
		//let area = Double.pi * pow(object.radius, 2)
		//var dragForceMagnitude = 0.5 * density * pow(object.velocity.length, 2) * dragCoefficient * area
		//if(dragForceMagnitude.isNaN) {
		//	dragForceMagnitude = 0
		//}
		//let dragForce = -object.velocity.normalized() * dragForceMagnitude
		//object.applyForce(force: dragForce, dt: dt)

		//let angularDragTorqueMagnitude = 8 * Double.pi * viscosity * pow(object.radius, 3) * object.spin.length / 3
		//let angularDragTorque = -object.spin.normalized() * angularDragTorqueMagnitude
		//object.applyTorque(torque: angularDragTorque, dt: dt)

		//let magnusForce = magnusForce(velocity: object.velocity, spin: object.spin, density: density, viscosity: viscosity, radius: object.radius)
		//object.applyForce(force: magnusForce, dt: dt)
		//print("drag: \(dragForceMagnitude), magnus: \(magnusForce.length), gravity: \(gravityForce.length), total: \((dragForce + magnusForce + gravityForce).length)")
		//object.applyHeat(heat: 
	}

	var collisions: [(Double, SphericalCow, SphericalCow)] = []
	var everything: [Moo] = entities + celestials
	for i in 0..<everything.count {
		for j in i+1..<everything.count {
			let object1 = everything[i].moo
			let object2 = everything[j].moo
			let deltaPosition = object2.position - object1.position
			let sumRadii = object1.radius + object2.radius
			let distance = deltaPosition.length - sumRadii
			let deltaVelocity = object2.velocity - object1.velocity
			let closingSpeed = -(deltaVelocity.dot(deltaPosition.normalized()))
			if distance > 0 && closingSpeed > 0 && distance < (closingSpeed * dt) {
				print("collision \(t): \(distance) \(closingSpeed) dt: \(dt)")
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
		let centerDistance = deltaPosition.length
		let collisionNormal = deltaPosition / centerDistance
		let normalVelocity = collisionNormal * (deltaVelocity.dot(collisionNormal))
		let tangentVelocity = deltaVelocity - normalVelocity
		let closingSpeed = normalVelocity.length
		let impulseMagnitude = abs(min(object1.mass, object2.mass) * closingSpeed * 2)
		let impulse = collisionNormal * impulseMagnitude
		object1.velocity -= impulse / object1.mass
		object2.velocity += impulse / object2.mass
		// I did consider advancing the movement by the same dt as I rewinded it earlier, but I think it's fine to
		// steal some time here, it's in the direction of the missing elasticity so it's kind of a good thing and it may
		// help reduce weirdness when three or more objects collide in the same tick.

		// rotation is still a bit messed up
		// This crappy algorithm doesn't compare difference in spin at the point of contact, just difference in spin.
		let collisionPoint = object1.position + (collisionNormal * object1.radius)
		let r1 = collisionPoint - object1.position
		let r2 = collisionPoint - object2.position
		let frictionCoefficient = (object1.frictionCoefficient * object2.frictionCoefficient)
		let rotationVelocity1 = object1.spin * object1.radius
		let rotationVelocity2 = object2.spin * object2.radius
		let totalRelativeTangentialVelocity = tangentVelocity + rotationVelocity2 - rotationVelocity1
		let minLinearMomentum = min(object1.mass, object2.mass) * totalRelativeTangentialVelocity.length
		let angularMomentum1 = object1.momentOfInertia * totalRelativeTangentialVelocity.length / object1.radius
		let angularMomentum2 = object2.momentOfInertia * totalRelativeTangentialVelocity.length / object2.radius
		let minAngularMomentum = min(angularMomentum1, angularMomentum2)
		let maxFrictionImpulseMagnitude = min(frictionCoefficient * impulseMagnitude * totalRelativeTangentialVelocity.length, minLinearMomentum + minAngularMomentum)
		let frictionImpulse = -totalRelativeTangentialVelocity.normalized() * maxFrictionImpulseMagnitude
		object1.velocity -= frictionImpulse * 0.5 / object1.mass
		object2.velocity += frictionImpulse * 0.5 / object2.mass
		let torque1 = r1.cross(frictionImpulse * 0.5)
		let torque2 = r2.cross(frictionImpulse * 0.5)
		let deltaSpin1 = torque1 / object1.momentOfInertia
		let deltaSpin2 = torque2 / object2.momentOfInertia
		object1.spin += deltaSpin1
		object2.spin -= deltaSpin2
		print("Collision Time:", String(format: "%f", elapsedTime))
		print("dvel:", deltaVelocity.format(4))
		print("dtan:", tangentVelocity.format(4))
		print("Impulse Magnitude:", String(format: "%.4f", impulseMagnitude))
		print("Impulse:", impulse.format(4))
		print("angular \(minAngularMomentum)")
		print("linear \(minLinearMomentum)")
		print("Friction Impulse:", frictionImpulse.format(4))
		print("dspin 1:", deltaSpin1.format(4))
		print("dspin 2:", deltaSpin2.format(4))
		print()

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
//		print("magnus force: \(magnusForce.format(2))")
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
