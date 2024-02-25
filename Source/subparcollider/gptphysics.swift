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

	init(id: Int64, position: Vector, velocity: Vector, orientation: Quaternion, spin: Vector, mass: Double, radius: Double, frictionCoefficient: Double = 0.1) {
		self.id = id
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
		return SphericalCow(id: id, position: position, velocity: velocity, orientation: orientation, spin: spin, mass: mass, radius: radius, frictionCoefficient: frictionCoefficient)
	}

	static func unit() -> SphericalCow {
		return SphericalCow(id: 0, position: Vector(), velocity: Vector(), orientation: Quaternion(), spin: Vector(), mass: 0, radius: 0, frictionCoefficient: 0)
	}

	// let's not do this for now, instead let's keep different star systems in separate coordinate spaces and not mix objects between them.
	func updateReferenceFrame(_ new: SphericalCow) {
		let old = self.referenceFrame ?? SphericalCow.unit()
		//self.position = self.position + old.position - new.position
		//self.velocity = self.velocity + old.velocity - new.velocity
		self.referenceFrame = new
	}

	// this function is not called by the collision code
	func applyForce(force: Vector, category: ForceCategory, dt: Double) {
		self.accumulatedForce += force
		if(category == .thrust) {
			warpVector += force * (dt / mass)
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
			object.moo.applyForce(force: gravity, category: .gravity, dt: dt)
			// these are celestials, they should all have the same frame of reference (their star's)
		}
		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)
	}
}

func collisionTest(_ object1: SphericalCow, _ object2: SphericalCow) -> (Double, Double) {
	let deltaPosition = object2.position - object1.position
	let sumRadii = object1.radius + object2.radius
	let distance = deltaPosition.length - sumRadii
	let deltaVelocity = object2.velocity - object1.velocity
	let closingSpeed = -(deltaVelocity.dot(deltaPosition.normalized()))
	return (distance, closingSpeed)
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
		let (gravity, nearest) = calculateGravities(subject: object.moo, objects: celestials)
		object.moo.applyForce(force: gravity, category: .gravity, dt: dt)
		if(nearest !== object.moo.referenceFrame) {
			object.moo.updateReferenceFrame(nearest)
		}
		let altitude = object.moo.position.length - nearest.radius
		let (density, _, viscosity) = atmosphericProperties(altitude: altitude)
		let dragCoefficient = 0.47 // Assuming a sphere
		let area = Double.pi * pow(object.moo.radius, 2)
		var dragForceMagnitude = 0.5 * density * pow(object.moo.velocity.length, 2) * dragCoefficient * area
		if(dragForceMagnitude.isNaN) {
			dragForceMagnitude = 0
		}
		let dragForce = -object.moo.velocity.normalized() * dragForceMagnitude
		object.moo.applyForce(force: dragForce, category: .impact, dt: dt)

		object.moo.integrateForce(dt: dt)
		object.moo.integrateTorque(dt: dt)
		if(object.c.b.count != object.sec.count) {
			object.createCows()
			object.recomputeCows()
		}

		object.updateCows()
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

	var easycollisions: [(Double, SphericalCow, SphericalCow)] = []
	var hardcollisions: [(Double, SphericalCow, SphericalCow, Entity)] = []
	var fmlcollisions: [(Double, SphericalCow, SphericalCow, Entity, Entity)] = []
	var everything: [Moo] = entities + celestials
	for i in 0..<celestials.count {
		for j in i+1..<celestials.count {
			let object1 = celestials[i].moo
			let object2 = celestials[j].moo
			let (distance, closingSpeed) = collisionTest(object1, object2)
			if distance > 0 && closingSpeed > 0 && distance < (closingSpeed * dt) {
				print("celestial-celestial collision \(t): \(distance) \(closingSpeed) dt: \(dt)")
				let collisionTime = distance / closingSpeed
				easycollisions.append((collisionTime, object1, object2))
			}
		}
	}
	for i in 0..<entities.count {
		let object1 = entities[i]
		let object2 = object1.moo.referenceFrame!
		let (distance, closingSpeed) = collisionTest(object1.moo, object2)
		if closingSpeed > 0 && distance < (closingSpeed * dt) {
			for k in 0..<object1.sec.count {
				let (distance2, closingSpeed2) = collisionTest(object1.sec[k].moo, object2)
				//if distance2 > 0 && closingSpeed2 > 0 && distance2 < (closingSpeed2 * dt) {
				if closingSpeed2 > 0 && distance2 < (closingSpeed2 * dt) {
					//print("ent-celestial collision t \(t) d \(distance2) v \(closingSpeed2) dt: \(dt)")
					let collisionTime2 = distance2 / closingSpeed2
					hardcollisions.append((collisionTime2, object1.sec[k].moo, object2, object1))
				}
			}
		}
	}
	for i in 0..<entities.count {
		for j in i+1..<entities.count {
			let object1 = entities[i]
			let object2 = entities[j]
			let (distance, closingSpeed) = collisionTest(object1.moo, object2.moo)
			if closingSpeed > 0 && distance < (closingSpeed * dt) {
				for k in 0..<object1.sec.count {
					for l in 0..<object2.sec.count {
						let (distance2, closingSpeed2) = collisionTest(object1.sec[k].moo, object2.sec[l].moo)
						if closingSpeed2 > 0 && distance2 < (closingSpeed2 * dt) {
							//print("ent-ent collision \(t): \(distance2) \(closingSpeed2) dt: \(dt)")
							//print("object1 section radius: \(object1.sec[k].moo.radius) object2 section radius: \(object2.sec[l].moo.radius)")
							let collisionTime2 = distance2 / closingSpeed2
							//fmlcollisions.append((collisionTime2, object1.sec[k].moo, object2.sec[l].moo, object1, object2))
							easycollisions.append((collisionTime2, object1.moo, object2.moo))
						}
					}
				}
			}
		}
	}

	let hpmult = 1000.0
	let invhpmult = 1.0/hpmult

	// one thing is still broken.
	// it was fine-ish when we ignored impulse angular component.
	hardcollisions.sort { $0.0 < $1.0 }
	for (elapsedTime, object1, object2, ent1) in hardcollisions {
		
/*
		let deltaPosition = object2.position - object1.position
		let deltaVelocity = object2.velocity - object1.velocity
		let centerDistance = deltaPosition.length
		let collisionNormal = deltaPosition / centerDistance
		let closingSpeed = -(deltaVelocity.dot(collisionNormal))
		let distance = centerDistance - object1.radius - object2.radius
		if (distance) < 0 || closingSpeed < 0 || distance > (closingSpeed * dt) {
			print("skipping")
			continue
		}
		let collisionPoint1 = object1.position + (collisionNormal * object1.radius)
		let collisionPoint2 = object2.position + (collisionNormal * object2.radius)
		let r1 = collisionPoint1 - ent1.moo.position
		let r2 = collisionPoint2 - object2.position
		let normalVelocity = collisionNormal * -closingSpeed
		let tangentVelocity = deltaVelocity - normalVelocity
		var impulseMagnitude = abs(min(ent1.moo.mass, object2.mass) * closingSpeed * 2)
		impulseMagnitude = min(object1.hp * hpmult / closingSpeed, object2.hp * hpmult / closingSpeed, impulseMagnitude)
		object1.hp -= closingSpeed * impulseMagnitude * invhpmult
		object2.hp -= closingSpeed * impulseMagnitude * invhpmult
		let impulse = collisionNormal * impulseMagnitude
		let impulseTorque = r1.cross(impulse * 0.5)
		var ke_lin = 0.5 * ent1.moo.mass * (closingSpeed ** 2)
		var ke_rot = 0.5 * ent1.moo.momentOfInertia * ((impulseTorque.length / ent1.moo.momentOfInertia) ** 2)
		let ke_sum = ke_lin + ke_rot
		print("kinetic energy rot, lin, sum", ke_rot, ke_lin, ke_sum)
		ke_lin *= ke_lin / ke_sum
		ke_rot *= ke_lin / ke_sum

		let dv = sqrt(2 * ke_lin / ent1.moo.mass)
		print("adjsted rot, lin, sum", ke_rot, ke_lin)
		let adjusted_impulse = (ent1.moo.mass * dv * 2) * collisionNormal
		let adjusted_impulseTorque = r1.cross(adjusted_impulse * 0.5)
		print("impulse \(impulse.length) adjusted \(adjusted_impulse.length) torque \(impulseTorque.length)")
		
		ent1.moo.velocity -= adjusted_impulse / ent1.moo.mass
		//ent1.moo.velocity -= impulse / ent1.moo.mass
		object2.velocity += impulse / object2.mass
		let frictionCoefficient = (object1.frictionCoefficient * object2.frictionCoefficient)
		let rotationVelocity1 = object1.spin.cross(collisionPoint1 - object1.position)
		let rotationVelocity2 = object2.spin.cross(r2)
		let totalRelativeTangentialVelocity = tangentVelocity + rotationVelocity1 + rotationVelocity2
		print("l: n \(normalVelocity.length) t \(tangentVelocity.length) r1 \(rotationVelocity1.length) r2 \(rotationVelocity2.length)")
		let magtanv = totalRelativeTangentialVelocity.length
		let minLinearMomentum = min(ent1.moo.mass, object2.mass) * magtanv
		let angularMomentum1 = ent1.moo.momentOfInertia * magtanv / ent1.moo.radius
		let angularMomentum2 = object2.momentOfInertia * magtanv / object2.radius
		let minAngularMomentum = min(angularMomentum1, angularMomentum2)
		var maxFrictionImpulseMagnitude = min(frictionCoefficient * impulseMagnitude * magtanv, minLinearMomentum, minAngularMomentum)
		maxFrictionImpulseMagnitude = min(object1.hp * hpmult / magtanv, object2.hp * hpmult / magtanv, maxFrictionImpulseMagnitude)
		object1.hp -= magtanv * maxFrictionImpulseMagnitude * invhpmult
		object2.hp -= magtanv * maxFrictionImpulseMagnitude * invhpmult
		let frictionImpulse = -totalRelativeTangentialVelocity.normalized() * maxFrictionImpulseMagnitude
		ent1.moo.velocity -= frictionImpulse * 0.5 / ent1.moo.mass
		object2.velocity += frictionImpulse * 0.5 / object2.mass
		let torque1 = r1.cross(frictionImpulse * 0.5)
		let torque2 = r2.cross(frictionImpulse * 0.5)
		let deltaSpin1 = (torque1 + adjusted_impulseTorque) / ent1.moo.momentOfInertia
		let deltaSpin2 = torque2 / object2.momentOfInertia
		ent1.moo.spin -= deltaSpin1
		object2.spin += deltaSpin2
*/

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
		let collisionPoint = object1.position + (collisionNormal * object1.radius)
		let r1 = collisionPoint - object1.position
		let r2 = collisionPoint - object2.position
		let frictionCoefficient = (object1.frictionCoefficient * object2.frictionCoefficient)
		let rotationVelocity1 = object1.spin.cross(r1) 
		let rotationVelocity2 = object2.spin.cross(r2)
		let totalRelativeTangentialVelocity = tangentVelocity + rotationVelocity2 - rotationVelocity1
		let maxLinearMomentum = min(object1.mass, object2.mass) * totalRelativeTangentialVelocity.length
		let angularMomentum1 = object1.momentOfInertia * totalRelativeTangentialVelocity.length / object1.radius
		let angularMomentum2 = object2.momentOfInertia * totalRelativeTangentialVelocity.length / object2.radius
		let maxAngularMomentum = min(angularMomentum1, angularMomentum2)
		let angularImpulse = -totalRelativeTangentialVelocity * maxAngularMomentum
		let maxFrictionImpulseMagnitude = min(frictionCoefficient * impulseMagnitude, maxLinearMomentum + maxAngularMomentum)
		let frictionImpulse = angularImpulse / angularImpulse.length * min(angularImpulse.length, maxFrictionImpulseMagnitude)
		object1.velocity -= frictionImpulse * 0.5 / object1.mass
		object2.velocity += frictionImpulse * 0.5 / object2.mass
		let torque1 = r1.cross(frictionImpulse * 0.5)
		let torque2 = r2.cross(frictionImpulse * 0.5)
		let deltaSpin1 = torque1 / object1.momentOfInertia
		let deltaSpin2 = torque2 / object2.momentOfInertia
		object1.spin -= deltaSpin1
		object2.spin += deltaSpin2

		let damage = closingSpeed * impulseMagnitude * invhpmult
		if(damage > 0.1) {
			object1.hp -= damage
			object2.hp -= damage
			ent1.dinged++
			/*print()
			print("hard collision Time:", String(format: "%f", elapsedTime))
			print("dvel:", deltaVelocity.format(4))
			print("dtan:", tangentVelocity.format(4))
			print("Impulse Magnitude:", String(format: "%.4f", impulseMagnitude))
			print("Impulse:", impulse.format(4))
			print("angular \(maxAngularMomentum)")
			print("linear \(maxLinearMomentum)")
			print("Friction Impulse:", frictionImpulse.format(4))
			print("dspin 1:", deltaSpin1.format(4))
			print("dspin 2:", deltaSpin2.format(4))
			print("object1 remaining hp: \(object1.hp) object2 remaining hp: \(object2.hp) ")*/
		}
	}

	easycollisions.sort { $0.0 < $1.0 }
	for (elapsedTime, object1, object2) in easycollisions {
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
		let collisionPoint = object1.position + (collisionNormal * object1.radius)
		let r1 = collisionPoint - object1.position
		let r2 = collisionPoint - object2.position
		let frictionCoefficient = (object1.frictionCoefficient * object2.frictionCoefficient)
		let rotationVelocity1 = object1.spin.cross(r1) 
		let rotationVelocity2 = object2.spin.cross(r2)
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
		object1.spin -= deltaSpin1
		object2.spin += deltaSpin2
/*
		print("easy collision Time:", String(format: "%f", elapsedTime))
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
*/
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
