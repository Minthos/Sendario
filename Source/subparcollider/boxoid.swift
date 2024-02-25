import Foundation

struct BoxoidCod: Codable {
	var corners: [Vector]
	var curvature: [Float]
	var material_idx: Int = 4
	var missing_faces: UInt = 0
	var bbox: BBox

	func elongationFactor(_ x: Double) -> Double {
		if(x > 0){
			return 1.0 + x
		} else {
			return 1.0 / (1.0 - x)
		}
	}

	mutating func bulgerize(_ v: Vector) {
		for i in 0..<6 {
			curvature[i*2] = curvature[i*2] + Float(v.x)
			curvature[i*2 + 1] = curvature[i*2+1] + Float(v.y)
		}
	}

	mutating func elongate(_ v: Vector) {
		let vx = Vector(elongationFactor(v.x), elongationFactor(v.y),  elongationFactor(v.z))
		for i in 0..<8 {
			corners[i] = (corners[i] - bbox.center) * vx + bbox.center
		}
		bbox = BBox(corners)
	}

	mutating func translate(_ v: Vector) {
		for i in 0..<8 {
			corners[i] = corners[i] + v
		}
		bbox = BBox(corners)
	}

	static func unit() -> BoxoidCod {
		var corners = [Vector((-1.0,  1.0, 1.0)),
							Vector((-1.0, -1.0, 1.0)),
							Vector((1.0, -1.0, 1.0)),
							Vector((1.0,  1.0, 1.0)),
							Vector((-1.0,  1.0, -1.0)),
							Vector((-1.0, -1.0, -1.0)),
							Vector((1.0, -1.0, -1.0)),
							Vector((1.0,  1.0, -1.0))]
		return BoxoidCod(corners: corners,
				  //curvature:[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
				  curvature:[1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
				  material_idx: 4, missing_faces: 0, bbox: BBox(corners))
	}
}

func convertCToSwift(boxoid: inout Boxoid) -> BoxoidCod {
	var corners: [Vector] = []
	var curvature: [Float] = []

	withUnsafePointer(to: &boxoid.corners) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 24) { $0 }
		for i in 0..<8 {
			let x = baseAddress[i*3]
			let y = baseAddress[i*3+1]
			let z = baseAddress[i*3+2]
			corners.append(Vector(Double(x), Double(y), Double(z)))
		}
	}

	withUnsafePointer(to: &boxoid.curvature) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 12) { $0 }
		curvature = Array(UnsafeBufferPointer(start: baseAddress, count: 12))
	}

	return BoxoidCod(corners: corners, curvature: curvature, material_idx: Int(boxoid.material_idx), missing_faces: UInt(boxoid.missing_faces), bbox: BBox(corners))
}

func convertSwiftToC(boxoidCod: BoxoidCod) -> Boxoid {
	var boxoid = Boxoid()
	withUnsafeMutablePointer(to: &boxoid.corners) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 24) { $0 }
		for i in 0..<8 {
			baseAddress[i*3] = Float(boxoidCod.corners[i].x)
			baseAddress[i*3+1] = Float(boxoidCod.corners[i].y)
			baseAddress[i*3+2] = Float(boxoidCod.corners[i].z)
		}
	}
	withUnsafeMutablePointer(to: &boxoid.curvature) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 12) { $0 }
		for i in 0..<12 {
			baseAddress[i] = Float(boxoidCod.curvature[i])
		}
	}
	boxoid.material_idx = Int32(boxoidCod.material_idx)
	boxoid.missing_faces = UInt32(boxoidCod.missing_faces)
	return boxoid
}

struct CompositeCod: Codable {
	var orientation: Quaternion
	var position: Vector
	var scale: Float
	var b: [BoxoidCod]
	var bbox: BBox
	
	func calculateBBox() -> BBox {
		if(b.count == 0) {
			return BBox(center: self.position, halfsize: 0)
		}
		var bbox = BBox.union(b.map { $0.bbox })
		return bbox
	}

	static func unit() -> CompositeCod {
		return CompositeCod(orientation: Quaternion(w: 0, x: 0, y: 1, z: 0), position: Vector(), scale: 0.667, b:[BoxoidCod.unit()], bbox: BBox())
	}

	static func load(_ path: String) -> CompositeCod {
		var JSONstr: String? = nil
		if FileManager.default.fileExists(atPath: path) {
			JSONstr = try! String(contentsOfFile: path, encoding: String.Encoding.utf8)
			return try!JSONDecoder().decode(self, from:JSONstr!.data(using: .utf8)!)
		} else {
			print("could not find \(path), starting with nothing")
			return unit()
		}
	}

	static func generateGrid(_ distance: Double) -> CompositeCod {
		var c = unit()
		c.b[0].elongate(Vector(distance, 10, 10))
		c.b.append(BoxoidCod.unit())
		c.b[1].elongate(Vector(10, distance, 10))
		c.b.append(BoxoidCod.unit())
		c.b[2].elongate(Vector(10, 10, distance))
		for i in 0..<26 {
			c.b.append(BoxoidCod.unit())
			c.b[3 + i * 3].elongate(Vector(distance, 10, 10))
			c.b.append(BoxoidCod.unit())
			c.b[4 + i * 3].elongate(Vector(10, distance, 10))
			c.b.append(BoxoidCod.unit())
			c.b[5 + i * 3].elongate(Vector(10, 10, distance))
		}
		for i in 0..<27 {
			for j in 0..<3 {
				for k in 0..<8 {
					c.b[i*3+j].corners[k] += (Vector(Double(i%3) - 1.0, Double((i/3) % 3) - 1.0, Double((i/9) % 3) - 1.0) * distance * 0.1)
				}
			}
		}
		c.bbox = c.calculateBBox()
		return c
	}
}

func fromC(_ composite: inout Composite) -> CompositeCod {
	var boxoids: [BoxoidCod] = []
	for i in 0..<composite.nb {
		boxoids.append(convertCToSwift(boxoid: &composite.b[i]))
	}
	var orientation = Quaternion(w: 0, x: 0, y: 0, z: 0)
	var position = Vector(0, 0, 0)
	withUnsafePointer(to: &composite.orientation) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 4) { $0 }
		orientation = Quaternion(w: Double(baseAddress[0]), x: Double(baseAddress[1]), y: Double(baseAddress[2]), z: Double(baseAddress[3]))
	}
	withUnsafePointer(to: &composite.position) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 3) { $0 }
		position = Vector(Double(baseAddress[0]), Double(baseAddress[1]), Double(baseAddress[2]))
	}
	var c = CompositeCod(orientation: orientation, position: position, scale: composite.scale, b: boxoids, bbox: BBox())
	c.bbox = c.calculateBBox()
	return c
}

func toC(_ compositeCod: CompositeCod) -> Composite {
	var composite = Composite()
	composite.b = UnsafeMutablePointer<Boxoid>.allocate(capacity: Int(compositeCod.b.count))
	for i in 0..<compositeCod.b.count {
		composite.b[Int(i)] = convertSwiftToC(boxoidCod: compositeCod.b[Int(i)])
	}
	withUnsafeMutablePointer(to: &composite.orientation) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 4) { $0 }
		baseAddress[0] = Float(compositeCod.orientation.w)
		baseAddress[1] = Float(compositeCod.orientation.x)
		baseAddress[2] = Float(compositeCod.orientation.y)
		baseAddress[3] = Float(compositeCod.orientation.z)
	}
	withUnsafeMutablePointer(to: &composite.position) { ptr in
		let baseAddress = ptr.withMemoryRebound(to: Float.self, capacity: 3) { $0 }
		baseAddress[0] = Float(compositeCod.position.x)
		baseAddress[1] = Float(compositeCod.position.y)
		baseAddress[2] = Float(compositeCod.position.z)
	}
	composite.scale = Float(compositeCod.scale)
	composite.nb = size_t(compositeCod.b.count)
	return composite
}

