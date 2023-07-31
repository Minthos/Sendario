import Foundation

struct Vector: Codable {
	var x: Double
	var y: Double
	var z: Double

	init() {
		self.x = 0
		self.y = 0
		self.z = 0
	}
   
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

	init(_ x: Double, _ y: Double, _ z: Double) {
		self.x = x
		self.y = y
		self.z = z
		sanityCheck()
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

	var float: (Float, Float, Float) {
		return (Float(x), Float(y), Float(z))
	}
	
	var spherical: SphericalVector {
		let rho = sqrt(x*x+y*y+z*z)
		let theta = atan(y/x)
		let phi = acos(z/sqrt(x*x+y*y+z*z))
		return SphericalVector(rho, theta, phi)
	}
	
	mutating func round(_ n: Double) {
		x = (x/n).rounded() * n
		y = (y/n).rounded() * n
		z = (z/n).rounded() * n
	}

	// round each coordinate up to nearest power of 2
	mutating func potimize() {
		x = potimizeDouble(x)
		y = potimizeDouble(y)
		z = potimizeDouble(z)
	}

	func xyz() -> [Double] {
		return [x, y, z]
	}
	
	var pretty: String {
		return "(\(x.pretty), \(y.pretty), \(z.pretty))"
	}
	
	var scientific: String {
		return "Vector(x: \(x.scientific), y: \(y.scientific), z: \(z.scientific))"
	}

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
	func cross(_ other: Vector) -> Vector {
		sanityCheck()
		other.sanityCheck()
		return Vector(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		)
	}

}

struct SphericalVector: Codable {
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
		return Vector(x, y, z)
	}
	  
	var pretty: String {
		return "(rho: \(rho.pretty), theta: \(theta.pretty), phi: \(phi.pretty))"
	}
	
	var scientific: String {
		return "SphericalVector(rho: \(rho.scientific), theta: \(theta.scientific), phi: \(phi.scientific))"
	}
}

struct Quaternion: Codable {
	var w: Double
	var x: Double
	var y: Double
	var z: Double

	init() {
		self.w = 1
		self.x = 0
		self.y = 0
		self.z = 0
	}
	
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

	// seems to have yaw and roll mixed up
	init(pitch: Double, yaw: Double, roll: Double)
	{
		let sinp = sin(pitch);
		let siny = sin(yaw);
		let sinr = sin(roll);
		let cosp = cos(pitch);
		let cosy = cos(yaw);
		let cosr = cos(roll);
		let w: Double = (cosr * cosp * cosy) + (sinr * sinp * siny)
		let x: Double = (sinr * cosp * cosy) - (cosr * sinp * siny)
		let y: Double = (cosr * sinp * cosy) + (sinr * cosp * siny)
		let z: Double = (cosr * cosp * siny) - (sinr * sinp * cosy)
		var tmp = Quaternion(w: w, x: x, y: y, z: z)
		let len_inv = 1.0 / tmp.length()
		self.w = tmp.w * len_inv
		self.x = tmp.x * len_inv
		self.y = tmp.y * len_inv
		self.z = tmp.z * len_inv
	}

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

	var xyz: Vector {
		return Vector(x, y, z)
	}

	func getAxisAngle() -> (Vector, Double) {
		let scale = 1.0 / sqrt(x * x + y * y + z * z)
		let axis = Vector(x * scale, y * scale, z * scale)
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
	
	var conjugate: Quaternion {
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
		let resQuat = left * vecQuat * left.conjugate
		return Vector(resQuat.x, resQuat.y, resQuat.z)
	}
}

prefix func --(value: inout Int) ->Int {
	value -= 1
	return value
}

prefix func ++(value: inout Int) -> Int {
	value += 1
	return value
}

postfix func --(value: inout Int) ->Int {
	defer { value -= 1 }
	return value
}

postfix func ++(value: inout Int) -> Int {
	defer { value += 1 }
	return value
}

prefix func - (vector: Vector) -> Vector {
	vector.sanityCheck()
	return Vector(-vector.x, -vector.y, -vector.z)
}
 
func *(lhs: Vector, rhs: Vector) -> Vector {
	return Vector(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z)
}


func /(lhs: Double, rhs: Vector) -> Vector {
	return rhs * lhs
}

func /(lhs: Vector, rhs: Double) -> Vector {
	lhs.sanityCheck()
	assert(rhs != 0)
	assert(!rhs.isNaN)
	return Vector(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs)
}

func +=(lhs: inout Vector, rhs: Vector) {
	lhs.sanityCheck()
	rhs.sanityCheck()
	lhs.x += rhs.x
	lhs.y += rhs.y
	lhs.z += rhs.z
}

func -=(lhs: inout Vector, rhs: Vector) {
	lhs.sanityCheck()
	rhs.sanityCheck()
	lhs.x -= rhs.x
	lhs.y -= rhs.y
	lhs.z -= rhs.z
}

func *=(lhs: inout Vector, rhs: Double) {
	lhs.sanityCheck()
	assert(!rhs.isNaN)
	lhs.x *= rhs
	lhs.y *= rhs
	lhs.z *= rhs
}

func /=(lhs: inout Vector, rhs: Double) {
	let inv = 1.0 / rhs
	lhs.x *= inv
	lhs.y *= inv
	lhs.z *= inv
	assert(rhs != 0)
	assert(!rhs.isNaN)
	lhs.sanityCheck()
}


func *(left: Double, right: Vector) -> Vector {
	right.sanityCheck()
	assert(!left.isNaN)
	return right * left
}

func *(left: Vector, right: Double) -> Vector {
	return Vector(left.x * right, left.y * right, left.z * right)
}

func +(left: Vector, right: Vector) -> Vector {
	return Vector(left.x + right.x, left.y + right.y, left.z + right.z)
}

func -(left: Vector, right: Vector) -> Vector {
	return Vector(left.x - right.x, left.y - right.y, left.z - right.z)
}

func +(left: Vector, right: Double) -> Vector {
	return Vector(left.x + right, left.y + right, left.z + right)
}

func -(left: Vector, right: Double) -> Vector {
	return Vector(left.x - right, left.y - right, left.z - right)
}

func ==(left: Vector, right: Vector) -> Bool {
	return left.x == right.x && left.y == right.y && left.z == right.z
}

func distance(_ left: Vector, _ right: Vector) -> Double {
	return sqrt((left.x - right.x) ** 2 + (left.y - right.y) ** 2 + (left.z - right.z) ** 2)
}

func distanceSquared(_ left: Vector, _ right: Vector) -> Double {
	return (left.x - right.x) ** 2 + (left.y - right.y) ** 2 + (left.z - right.z) ** 2
}

precedencegroup ExponentiationPrecedence {
  associativity: left
  higherThan: MultiplicationPrecedence
}
infix operator ** : ExponentiationPrecedence

func ** (num: Double, power: Double) -> Double {
	return pow(num, power)
}

extension Double {
	static var epsilon: Double { return Double.leastNonzeroMagnitude }
}

extension Formatter {
	static let scientific: NumberFormatter = {
		let formatter = NumberFormatter()
		formatter.numberStyle = .scientific
		formatter.positiveFormat = "0.###E+0"
		formatter.exponentSymbol = "e"
		return formatter
	}()
}

extension Numeric {
	var scientific: String {
		return Formatter.scientific.string(for: self) ?? ""
	}
}

extension Double {
	var pretty: String {
		var s = ""
		if(self < 0.001) {
			return self.scientific
		} else if(self < 1) {
			s = String(format: "%.4f", self)
		}  else if(self < 1000) {
			s = String(format: "%.2f", self)
		}  else if(self < 1e6) {
			s = String(format: "%.2fk", self/1e3)
		}  else if(self < 1e9) {
			s = String(format: "%.2fM", self/1e6)
		}  else if(self < 1e12) {
			s = String(format: "%.2fG", self/1e9)
		}  else if(self < 1e15) {
			s = String(format: "%.2fT", self/1e12)
		} else {
			return self.scientific
		}
		var snew = ""
		var done = false
		for (_, c) in s.enumerated().reversed() {
			if(done) {
				snew.append(c)
			} else if(c == "0") {
				continue;
			} else if c.isNumber {
				done = true
				snew.append(c)
			} else if c == "." {
				done = true
			} else {
				snew.append(c)
			}
		}
		return String(snew.reversed())
	}
}

extension Sequence where Element: AdditiveArithmetic {
	func sum() -> Element { reduce(.zero, +) }
}

struct Lazy<T> {
	private var lambda: () -> T
	lazy var v: T = lambda()
	init(_ lambda:  @escaping @autoclosure () -> T) { self.lambda = lambda }
}

extension Array {
	@inlinable
	public mutating func popFirst() -> Element? {
		if isEmpty {
			return nil
		} else {
			return removeFirst()
		}
	}
}

// round up to nearest power of 2
func potimizeDouble(_ number: Double) -> Double {
	if(number.binade == number) {
		return number
	} else {
		return number.binade * 2
	}
}

func hexString(_ number: UInt64) -> String {
	return String(format: "%llx", number)
}

struct BBox: Codable {
	var center: Vector
	var halfsize: Double

	var top: Vector { get { return center + halfsize } }
	var bottom: Vector { get { return center - halfsize } }
	var pretty: String { get { return center.pretty + " halfsize: " + halfsize.pretty }}

	init() {
		center = Vector()
		halfsize = 0
	}

	init(center: Vector, halfsize: Double) {
		self.center = center
		self.halfsize = halfsize
	}

	init(top: Vector, bottom: Vector) {
		self.center = (top + bottom) * 0.5
		self.halfsize = abs(top.x - bottom.x) * 0.5
	}
	
	init(_ points: [Vector]) {
		assert(points.count > 0)
		var top = points[0]
		var bottom = points[0]
		for i in 1..<points.count {
			top.x = max(points[i].x, top.x)
			bottom.x = min(points[i].x, bottom.x)
			top.y = max(points[i].y, top.y)
			bottom.y = min(points[i].y, bottom.y)
			top.z = max(points[i].z, top.z)
			bottom.z = min(points[i].z, bottom.z)
		}
		self.center = (top + bottom) * 0.5
		self.halfsize = max(abs(top.x - bottom.x), abs(top.y - bottom.y), abs(top.z - bottom.z)) * 0.5
	}

	static func union(_ bbox: [BBox]) -> BBox {
		return BBox(bbox.flatMap { b in return [b.top, b.bottom] } )
		//return	BBox([bbox.top, bbox.bottom, self.top, self.bottom])
	}

	// returns a (1/4, 1/4, 1/4) size section of a bounding box.
	// Which of the 64 possible subsections is indicated by the argument "index"
	func selectQuadrant(_ index: UInt8) -> BBox {
		let xindex = Double(index & 0x03)
		let yindex = Double((index >> 2) & 0x03)
		let zindex = Double((index >> 4) & 0x03)
		let quartersize = halfsize * 0.5
		let eighthsize = halfsize * 0.25
		let offset = eighthsize - halfsize
		return BBox(center: Vector(center.x + offset + xindex * quartersize,
								  center.y + offset + yindex * quartersize,
								  center.z + offset + zindex * quartersize),
					halfsize: eighthsize)
	}

	func contains(_ point: Vector) -> Bool {
		return( abs(point.x - center.x) < halfsize &&
				abs(point.y - center.y) < halfsize &&
				abs(point.z - center.z) < halfsize )
	}

	func intersects(bbox: BBox) -> Bool {
		// Radius isn't the right word. Suggestions welcome.
		let collisionRadius = bbox.halfsize + halfsize
		return( abs(bbox.center.x - center.x) < collisionRadius &&
				abs(bbox.center.y - center.y) < collisionRadius &&
				abs(bbox.center.z - center.z) < collisionRadius )
	}

	var scientific: String {
		return "BBox(top: \(top.scientific), bottom: \(bottom.scientific))"
	}
}


