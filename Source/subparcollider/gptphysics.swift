import Foundation

// leave the collision code as it is for now, elastic collisions will solve our problems
// reentry heat and drag
class SphericalCow: Codable {
	let id: Int64
	var referenceFrame: SphericalCow? = nil
	var position: Vector
	var w: Double
	var warpVector: Vector
	var FTL: Bool
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
	var hp: Double = 0
	var armor: Double = 0
	var kineticEnergy: Double { return (mass * 0.5 * velocity.lengthSquared) + (momentOfInertia * 0.5 * spin.lengthSquared) }

	init(id: Int64, referenceFrame: SphericalCow?, position: Vector, velocity: Vector, orientation: Quaternion, spin: Vector, mass: Double, radius: Double, frictionCoefficient: Double = 0.1) {
		self.id = id
		self.referenceFrame = referenceFrame
		self.position = position
		self.w = 200.0
		self.warpVector = Vector()
		self.FTL = false
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
		return SphericalCow(id: id, referenceFrame: referenceFrame, position: position, velocity: velocity, orientation: orientation, spin: spin, mass: mass, radius: radius, frictionCoefficient: frictionCoefficient)
	}

	static func unit() -> SphericalCow {
		return SphericalCow(id: 0, referenceFrame: nil, position: Vector(), velocity: Vector(), orientation: Quaternion(), spin: Vector(), mass: 0, radius: 0, frictionCoefficient: 0)
	}

	func updateReferenceFrame(_ new: SphericalCow) {
		let old = self.referenceFrame ?? SphericalCow.unit()
		self.position = self.position - old.position + new.position
		self.velocity = self.velocity - old.velocity + new.velocity
		self.referenceFrame = new
	}

	func applyForce(force: Vector, category: ForceCategory, dt: Double) {
		self.accumulatedForce += force
		if(category == .thrust) {
			warpVector += force * (dt / mass)
		} else if(category == .drag) {
			heat += dt * force.length / mass
		}
	}

	func applyTorque(torque: Vector, category: ForceCategory, dt: Double) {
		self.accumulatedTorque += torque
	}

	func integrateForce(dt: Double) {
		let new_pos = position + velocity * dt + (prevForce / mass) * (dt * dt * 0.5)
		let sum_accel = (prevForce + accumulatedForce) / mass
		let new_vel = velocity + (sum_accel)*(dt*0.5)
		position = FTL ? (new_pos + warpVector * (w * w * dt)) : new_pos
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
		}
		let sum_rotAccel = (prevTorque + accumulatedTorque) / momentOfInertia
		let new_spin = spin + (sum_rotAccel)*(dt*0.5)
		spin = new_spin
		prevTorque = accumulatedTorque
		accumulatedTorque = Vector(0, 0, 0)
	}

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

func calculatePeriApo(_ moo: SphericalCow) -> (SphericalVector, SphericalVector) {
	return(SphericalVector(0,0,0), SphericalVector(0,0,0))
}

// calculate the strongest single gravity force affecting an object unless the object is inside a celestial
func calculateGravity(subject: SphericalCow, objects: [Celestial]) -> (Vector, SphericalCow) {
	var gravity = Vector(0, 0, 0)
	var strongestForce = 0.0
	var strongestAttractor = subject
	for object in objects {
		var deltaPosition = -subject.position
		if(object.moo !== subject.referenceFrame) {
			deltaPosition += object.moo.position - (subject.referenceFrame?.position ?? Vector())
		}
		let distance = deltaPosition.length
		var mass = object.moo.mass
		if(distance < 0.001) {
			return(Vector(), object.moo)
		} else if(distance < object.moo.radius) {
			let volumeEnclosed = (4.0 / 3.0) * Double.pi * pow(distance, 3)
			mass = object.moo.density * volumeEnclosed
			let forceMagnitude = (G * mass * subject.mass) / pow(distance, 2)
			gravity = deltaPosition.normalized() * forceMagnitude
			return (gravity, object.moo)
		} else {
			let forceMagnitude = (G * mass * subject.mass) / pow(distance, 2)
			if forceMagnitude > strongestForce {
				gravity += deltaPosition.normalized() * forceMagnitude
				strongestForce = forceMagnitude
				strongestAttractor = object.moo
			}
		}
	}
	return (gravity, strongestAttractor)
}

// calculate the sum of gravity forces affecting a celestial
func calculateGravities(subject: SphericalCow, objects: [Celestial]) -> Vector {
	var gravity = Vector(0, 0, 0)
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
		}
	}
	return gravity
}

func gravTick(center: SphericalCow, celestials: inout [Celestial], t: Double, dt: Double) {
	for object in celestials {
		if(object.moo !== center){
			let gravity = calculateGravities(subject: object.moo, objects: celestials)
			object.moo.applyForce(force: gravity, category: .gravity, dt: dt)
			// these are celestials, they should all have the same frame of reference (nil, implicitly centered on the star they orbit)
		}
		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)
	}
}

func collisionTest(_ object1: SphericalCow, _ object2: SphericalCow) -> (Double, Double) {
	let deltaPosition = object2.position - object1.position
	let sumRadii = object1.radius + object2.radius
	let centerDistance = deltaPosition.length
	var distance = centerDistance - sumRadii
	if(distance < 0) {
		let awayVector = deltaPosition.normalized() * (distance * 3)
		object1.position += awayVector
		object2.position -= awayVector
		distance = distance * -3
	}
	let deltaVelocity = object2.velocity - object1.velocity
	let collisionNormal = deltaPosition / centerDistance
	let normalVelocity = collisionNormal * (deltaVelocity.dot(collisionNormal))
	let closingSpeed = normalVelocity.length
	if(closingSpeed > deltaVelocity.length * 1.001) {
		print("closingSpeed: \(closingSpeed), deltaVelocity.length: \(deltaVelocity.length)")
	}
	return (distance, closingSpeed)
}

func applyCollision(_ elapsedTime: Double, _ closingSpeed: Double, _ object1: inout SphericalCow, _ object2: inout SphericalCow) {
	let deltaPosition = object2.position - object1.position
	let deltaVelocity = object2.velocity - object1.velocity
	let centerDistance = deltaPosition.length
	let collisionNormal = deltaPosition / centerDistance
	let normalVelocity = collisionNormal * (deltaVelocity.dot(collisionNormal))
	let tangentVelocity = deltaVelocity - normalVelocity
	let impulseMagnitude = min(object1.mass, object2.mass) * closingSpeed * 2
	var impulseMag1 = impulseMagnitude / object1.mass
	var impulseMag2 = impulseMagnitude / object2.mass
	if(impulseMag1 < impulseMag2) {
		impulseMag2 = impulseMagnitude - impulseMag1
	} else {
		impulseMag1 = impulseMagnitude - impulseMag2
	}
	let bounciness = 0.7
	let dv1 = collisionNormal * -impulseMag1 * bounciness / object1.mass
	let dv2 = collisionNormal * impulseMag2 * bounciness / object2.mass
	if(dv1.length > deltaVelocity.length * 2.1 || dv2.length > deltaVelocity.length * 2.1) {
		print("shit's fucked yo \(dv1.length) \(dv2.length) \(deltaVelocity.length * 2)")
		print("easy collision Time:", String(format: "%f", elapsedTime))
		print("dvel:", deltaVelocity.format(4))
		print("dtan:", tangentVelocity.format(4))
		print("Impulse Magnitude:", String(format: "%.4f", impulseMagnitude))
		print()
		object1.velocity = Vector()
		object2.velocity = Vector()
		assert(false)
		exit(-1)
	}

	object1.velocity += dv1
	object2.velocity += dv2
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
		action.object.applyForce(force: action.force, category: .thrust, dt: dt)
		action.object.applyTorque(torque: action.torque, category: .thrust, dt: dt)
	}

	for object in entities {
		let (gravity, nearest) = calculateGravity(subject: object.moo, objects: celestials)
		if(nearest !== object.moo.referenceFrame) {
			object.moo.updateReferenceFrame(nearest)
		}
		object.moo.applyForce(force: gravity, category: .gravity, dt: dt)
		let altitude = object.moo.position.length - nearest.radius
		let (density, _, viscosity) = atmosphericProperties(altitude: altitude)
		let dragCoefficient = 0.47 // Assuming a sphere
		let area = Double.pi * pow(object.moo.radius, 2)
		var dragForceMagnitude = 0.5 * density * pow(object.moo.velocity.length, 2) * dragCoefficient * area
		if(dragForceMagnitude.isNaN) {
			dragForceMagnitude = 0
		}
		let dragForce = -object.moo.velocity.normalized() * dragForceMagnitude
		object.moo.applyForce(force: dragForce, category: .drag, dt: dt)
		// TODO: dissipate heat, apply damage from heat. right now heat only ever increases, which is fine since it does nothing.
		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)

		if(object.c.b.count != object.sec.count) {
			object.createCows()
			object.recomputeCows()
		}

//		object.updateCows()
	}

	var collisions: [(Double, Double, SphericalCow, SphericalCow)] = []
	for i in 0..<celestials.count {
		for j in i+1..<celestials.count {
			var object1 = celestials[i].moo
			var object2 = celestials[j].moo
			let (distance, closingSpeed) = collisionTest(object1, object2)
			if distance > 0 && closingSpeed > 0 && distance < (closingSpeed * dt) {
				print("celestial-celestial collision \(t): \(distance) \(closingSpeed) dt: \(dt)")
				let collisionTime = distance / closingSpeed
				applyCollision(collisionTime, closingSpeed, &object1, &object2)
			}
		}
	}
	for i in 0..<entities.count {
		let object1 = entities[i]
		// clever trick to prevent us from messing with the planet/moon when calculating collisions with the ground
		var object2 = object1.moo.referenceFrame!.copy()
		object2.position = Vector()
		object2.velocity = Vector()
		object2.spin = Vector()
		let (distance, closingSpeed) = collisionTest(object1.moo, object2)
		if closingSpeed > 0 && distance >= 0 && distance < (closingSpeed * dt) {
			var collisionTime = distance/closingSpeed
			assert(collisionTime <= dt)
			if(collisionTime < 0.0) {
				collisionTime = 0.0
			}
			applyCollision(collisionTime, closingSpeed, &object1.moo, &object2)
		}
	}
	for i in 0..<entities.count {
		for j in i+1..<entities.count {
			var object1 = entities[i]
			var object2 = entities[j]
			let (distance, closingSpeed) = collisionTest(object1.moo, object2.moo)
			if closingSpeed > 0 && distance >= 0 && distance < (closingSpeed * dt) {
				var collisionTime = distance / closingSpeed
				assert(collisionTime <= dt)
				if(collisionTime < 0.0) {
					collisionTime = 0.0
				}
				applyCollision(collisionTime, closingSpeed, &object1.moo, &object2.moo)
			}
		}
	}
}

