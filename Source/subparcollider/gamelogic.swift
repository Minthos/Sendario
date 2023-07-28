
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
	case crudeGas
	case crudeOre
	case slag
	case scrap
	case gas
	case mineral
	case propellant
	case heavy_isotopes
	case light_isotopes
	case antimatter
	case components
	case currency
	case compute
	case ACRound
	case ultraACRound
	case dumbMissile
	case homingMissile
	case proximityMine
	case kineticWarhead
	case explosiveWarhead
	case flakWarhead
	case smokeWarhead
	case nuclearWarhead
	case fusionWarhead
	case antimatterWarhead

	// exotic ingredients
	case melange
	case unobtainium
	case kryptonite
	case fairy_dust
	case spider_silk
	case mithril
	case angel_hair
	case unicorn_farts
	case dragon_scales
	case plutonic_quartz
	case happiness
	case carebear_tears
	case short_shorts
	case schadenfreude
	case albino_butterflies
	case vampire_dust
	case minotaur_hoof_clippings
	case ectoplasm
	case grey_goo
	case dark_matter
	case beholder_eyelashes
	case shoggoth_poop
	case alien_fossils
	case antientropy
	case classic_vinyl_records
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
	let bonuses: [(String, Double)]
	let maluses: [(String, Double)]
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
	case heatSink
	case battery
	case antimatterBattery
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
}

class Section: Codable, Moo {
	weak var ent: Entity
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

	init(pent: Entity, pbindex: Int, ptype: SectionType) {
		ent = pent
		bindex = pbindex
		type = ptype
		moo = ent.moo.copy() // TODO compute actual values instead
		computeValues()
	}

	// TODO compute actual values instead
	func computeValues() {
		externalVolume = ent.b[bindex].bbox.halfsize ** 3
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
			case .heatSink:
				hp *= 20.0
			case .battery:
				hp *= 20.0
			case .antimatterBattery:
				hp *= 5.0
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
		}
		hp = hp + armorAmount * 200 + structureAmount * 100
	}
}

class Entity: Codable, Moo {
	var name: String
	var moo: SphericalCow
	var c: CompositeCod
	var sec: [Section]
	
	init(name: String, _ moo: SphericalCow) {
		self.name = name
		self.moo = moo
		self.c = CompositeCod.unit()
		self.bmoo = []
	}

	func createCows() {
		for i in bmoo.count..<c.b.count {
			bmoo.append(moo.copy())
		}
	}

	func recomputeCows() {
		for (i, box) in c.b.enumerated() {
			bmoo[i].radius = box.bbox.halfsize
			bmoo[i].hp = 100 * moo.mass / Double(c.b.count)
		}
	}

	func updateCows() {
		for (i, box) in c.b.enumerated() {
			// quaternion transforms incoming, bugs incoming
			let boxCenterRotated = moo.orientation * Quaternion(w:0, x: box.bbox.center.x, y: box.bbox.center.y, z: box.bbox.center.z) * moo.orientation.conjugate
			bmoo[i].position = moo.position + boxCenterRotated.xyz
			bmoo[i].velocity = moo.velocity + moo.spin.cross(boxCenterRotated.xyz)
			bmoo[i].spin = moo.spin
			bmoo[i].orientation = moo.orientation
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
	case impact // the impact forces are not reported through the correct channels.
}

