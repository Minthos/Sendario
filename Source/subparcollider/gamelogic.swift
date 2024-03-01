
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

enum CargoType: Codable {
	// standard cargo
	case regolith// minerals and slag
	case mineral// iron, carbon, aluminium, calcium etc.
	case scrap// broken items, can be refined for minerals and components
	case slag// waste material with little value
	case crudeIce// frozen mix of gas and regolith
	case crudeGas// mixed gases
	case gas// purified gas
	case propellant// various liquids and gases with low reactivity
	case oxidizer// usually oxygen
	case fuel// hydrocarbons and other reactive liquids and gases
	case heavy_isotopes// fissiles
	case light_isotopes// fusiles
	case antimatter// antimatter in magnetic confinement canisters. will release all its energy if any canister is damaged.
	case components// various electronic and mechanical components
	case currency// physical currency
	case compute// powerful computer chips designed with redundancy to protect against bit errors
	case ACRound// a cartridge with propellant, should be combined with a warhead before use
	case ultraACRound// a cartridge with propellant, should be combined with a warhead before use
	case dumbMissile// a missile with propellant, should be combined with a warhead before use
	case homingMissile// a missile with propellant and guidance, should be combined with a warhead before use
	case proximityMine// a mine, should be combined with a warhead before use
	case kineticWarhead// kinetic penetrator, the only warhead mass drivers can use and a cheap warhead for autocannons
	case explosiveWarhead// warhead with chemical explosives. explodes if destroyed.
	case flakWarhead// warhead with chemical exploives and tiny kinetic/incendiary fragments. explodes if destroyed.
	case smokeWarhead// releases smoke. good defense against beam weapons and enemy sensors
	case nuclearWarhead// big boom. minimum size 20kg, maximum 1 t. a single blast can destroy a spaceship. big AOE.
	case fusionWarhead// minimum size 100kg, maximum 10 t. when a big boom is too small. big AOE.
	case antimatterWarhead// the biggest boom. minimum size 1kg. explodes if destoyed. big AOE.

	// exotic ingredients: all can improve upgrades in addition to any other properties they have
	case heisenbergite// crystal required for gaslight drives. found in small quantities in meteor craters in every star system (0 rarity)
	case dark_matter// (1 rarity) most commonly found floating around in the interstellar medium
	case alien_artifacts// (1 rarity)
	case superconductors// (1 rarity) reduces waste heat
	case classic_vinyl_records// (2 rarity)
	case ectoplasm// (2 rarity) ghosts live underground
	case albino_butterflies// (2 rarity) butterflies live aboveground
	case alien_fossils// (2 rarity)
	case mithril// weight reduction, strength increase (3 rarity)
	case plutonic_quartz// increases ftl drive warp strength (3 rarity)
	case shoggoth_poop// (3 rarity)
	case grey_goo// (3 rarity)
	case green_goop// (3 rarity)
	case vampire_dust// (4 rarity) vampires live underground
	case unobtainium// when used as nuclear fuel it lasts forever (4 rarity)
	case fairy_dust// (4 rarity) fairies live aboveground
	case spider_silk// weight reduction, strength increase (4 rarity)
	case unicorn_farts// when used as fusion fuel it lasts forever (4 rarity)
	case melange// hallucinogen. only found on the most arid desert planets. (5 rarity)
	case kryptonite// strength increase, improves kinetic penetrator damage. (5 rarity)
	case angel_hair// regenerative properties (5 rarity)
	case dragon_scales// improves heat tolerance (5 rarity)
	case minotaur_horns// (5 rarity) minotaurs live underground
	case beholder_eyelashes// (5 rarity) beholders live underground
	case antientropy// (5 rarity)
	case gargoyle_skin// (5 rarity)
}

class Cargo: Codable {
	var mass: Double
	let type: CargoType
	let level: Int
}

struct Upgrade: Codable {
	let blueprint: BoxoidCod
	let blueprintType: SectionType
	let level: Int
	let ingredients: [Cargo]
	let bonuses: Dictionary<String, Double>
	let maluses: Dictionary<String, Double>
}

enum SectionType: Codable {
	case arcjetThruster
	case chemicalThruster
	case ionThruster
	case nuclearThruster
	case fusionThruster
	case gaslightDrive
	case nuclearReactor
	case fusionReactor
	case antimatterReactor
	case heatSink
	case battery
	case tank
	case cargoHold
	case hull
	case droneBay
	case turret
	case massDriver
	case autocannon
	case beam
	case missileLauncher
	case gammaCannon
	case hwFactory
	case chipFab
	case bioFactory
	case mineralRefinery
	case gasRefinery
	case VAB
	case sensors
	case lifeSupport
	case spaceElevator
}

class Section: Codable, Moo {
	weak var ent: Entity!
	var moo: SphericalCow
	var bindex: Int
	var type: SectionType
	var level: Int = 0
	var upgrades: [Upgrade] = []
	var externalVolume: Double = 0.0
	var internalVolume: Double = 0.0
	var structureAmount: Double = 0.0
	var armorUpgrades: [Upgrade] = []
	var armorLevel: Int = 0
	var armorAmount: Double = 0.0
	var hp: Double = 0.0

	private enum CodingKeys: CodingKey {
   		case moo
		case bindex
		case type
		case level
		case upgrades
		case externalVolume
		case internalVolume
		case structureAmount
		case armorUpgrades
		case armorLevel
		case armorAmount
		case hp
	}

	init(_ pent: Entity, _ pbindex: Int, _ ptype: SectionType) {
		ent = pent
		bindex = pbindex
		type = ptype
		moo = ent.moo.copy() // this can be improved a lot
		computeValues()
	}

	func computeValues() {
		// this can be improved a lot
		externalVolume = ent.c.b[bindex].bbox.halfsize ** 3
		internalVolume = externalVolume - structureAmount - armorAmount
		hp = externalVolume
		switch(type) {
			case .arcjetThruster:
				hp *= 5.0
			case .chemicalThruster:
				hp *= 5.0
			case .ionThruster:
				hp *= 5.0
			case .nuclearThruster:
				hp *= 20.0
			case .fusionThruster:
				hp *= 10.0
			case .gaslightDrive:
				hp *= 20.0
			case .nuclearReactor:
				hp *= 10.0
			case .fusionReactor:
				hp *= 20.0
			case .antimatterReactor:
				hp *= 5.0
			case .heatSink:
				hp *= 20.0
			case .battery:
				hp *= 20.0
			case .tank:
				hp *= 50.0
			case .cargoHold:
				hp *= 100.0
			case .hull:
				hp *= 100.0
			case .droneBay:
				hp *= 50.0
			case .turret:
				hp *= 50.0
			case .massDriver:
				hp *= 50.0
			case .autocannon:
				hp *= 50.0
			case .beam:
				hp *= 10.0
			case .missileLauncher:
				hp *= 5.0
			case .gammaCannon:
				hp *= 200.0
			case .hwFactory:
				hp *= 50.0
			case .chipFab:
				hp *= 5.0
			case .bioFactory:
				hp *= 50.0
			case .mineralRefinery:
				hp *= 50.0
			case .gasRefinery:
				hp *= 50.0
			case .VAB:
				hp *= 50.0
			case .sensors:
				hp *= 20.0
			case .lifeSupport:
				hp *= 20.0
			case .spaceElevator:
				hp *= 200.0
		}
		hp = hp + armorAmount * 200 + structureAmount * 100
	}
}

class Entity: Codable, Moo {
	var name: String
	var moo: SphericalCow
	var inertia: Matrix3
	var c: CompositeCod
	var sec: [Section]
	var dinged: Int
	
	init(name: String, _ moo: SphericalCow) {
		self.name = name
		self.moo = moo
		self.c = CompositeCod.unit()
		self.c.scale = Float(moo.radius)
		self.sec = []
		self.dinged = 0
		self.inertia = Matrix3()
	}

	func createCows() {
		for i in sec.count..<c.b.count {
			sec.append(Section(self, i, .hull))
		}
	}

	// this runs after the ship has been modified in the workshop
	func recomputeCows() {
		moo.hp = 0
		moo.mass = 0
		for (i, box) in c.b.enumerated() {
			sec[i].moo.radius = box.bbox.halfsize
			sec[i].moo.mass = (box.bbox.halfsize * 2) ** 3
			sec[i].moo.hp = sec[i].hp
			moo.hp += sec[i].hp
			moo.mass += sec[i].moo.mass
		}
		c.bbox = c.calculateBBox()
		moo.radius = c.bbox.halfsize
		moo.momentOfInertia = 2 * moo.mass * moo.radius * moo.radius / 5
	}

	// this will run after a tick has completed
	func damageReport() -> Bool {
		var spin = moo.spin
		self.updateVelocityAndSpinAfterCollision()
		self.updateCows()
		let initialHp = moo.hp
		moo.hp = 0
		moo.mass = 0
		var keepers: [Int] = []
		for (i, box) in c.b.enumerated() {
			if(sec[i].moo.hp >= 0.001) {
				keepers.append(i)
				moo.hp += sec[i].moo.hp
				moo.mass += sec[i].moo.mass
			} else {
				sec[i].moo.mass = 0
			}
		}
		moo.momentOfInertia = 2 * moo.mass * moo.radius * moo.radius / 5
		if(initialHp != moo.hp) {
		//	print("damage report (\(sec.count) sections):\ninitial hp: \(initialHp)")
		//	print("damage taken: \(initialHp - moo.hp)")
		//	print("remaining hp: \(moo.hp)")
		}

		// I disabled destruction because it causes segfault
		//if keepers.count != sec.count {
		if false {
			print("lost \(sec.count - keepers.count) section(s)")
			var newb: [BoxoidCod] = []
			var news: [Section] = []
			for i in 0..<keepers.count {
				newb.append(c.b[keepers[i]])
				news.append(sec[keepers[i]])
			}
			c.b = newb
			sec = news
			c.bbox = c.calculateBBox()
			spin = moo.spin
			// we do this to recompute the inertia tensor. if there are no bugs it should not alter velocity or spin 🙃
			self.updateVelocityAndSpinAfterCollision()
			assert((moo.spin - spin).length < 0.01)
			self.updateCows()
			return true
		}
		return false
	}

	// progress report:
	// overallAngularMomentum seems reasonable but we need to keep track of:
	// ongoing collisions: elastic collisions and objects penetrating each other
	// multiple boxoids from the same composite hitting the same object in the same tick

	// actually this is probably a good place to do something with elasticity. The elastic force between a boxoid
	// and its composite should be added to that boxoid's impulse calculation for the following tick so it repels
	// the ground and other objects with a force that equals the force it exerts on the composite plus its own inertia
	func updateVelocityAndSpinAfterCollision() {
		if(self.sec.count == 0) {
			return
		} else if(self.sec.count == 1) {
			self.moo.velocity = self.sec[0].moo.velocity
			self.moo.spin = self.sec[0].moo.spin
			return
		}
		var inertiaTensor = Matrix3()
		var overallMomentum = Vector()
		var overallAngularMomentum = Vector()
		var avgSpin = Vector()
		for (i, section) in self.sec.enumerated() {
			let pos = c.b[i].bbox.center
			let mass = section.moo.mass
			inertiaTensor.addInertiaTensorContribution(pos: pos, mass: mass)
			let sectionMomentum = mass * section.moo.velocity
			overallMomentum += sectionMomentum
			let sectionRelativeMomentum = mass * (section.moo.velocity - self.moo.velocity)
			overallAngularMomentum += pos.cross(sectionRelativeMomentum)// + (section.moo.spin * section.moo.momentOfInertia)
		}
		self.inertia = inertiaTensor
		self.moo.velocity = overallMomentum / self.moo.mass
		self.moo.spin = inertiaTensor.inverse() * overallAngularMomentum
	}

	func updateCows() {
		for (i, box) in c.b.enumerated() {
			// quaternion transform, shiny!
			let boxCenterRotated = moo.orientation * Quaternion(w:0, x: box.bbox.center.x, y: box.bbox.center.y, z: box.bbox.center.z) * moo.orientation.conjugate
			sec[i].moo.position = moo.position + boxCenterRotated.xyz
			sec[i].moo.velocity = moo.velocity + moo.spin.cross(boxCenterRotated.xyz)
			// should calculate centrifugal force and tear off boxoids whose centrifugal > centripetal
			sec[i].moo.spin = moo.spin
			sec[i].moo.orientation = moo.orientation
		}
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

enum ForceCategory {
	case thrust
	case gravity
	case impact
	case drag
}

