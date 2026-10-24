//
//  BoardGeometry.swift
//  Cheech
//
//  Copyright (c) 2005-2026 Ben Levitt
//
//  SPDX-License-Identifier: MIT
//

import CoreGraphics
import Foundation
import SwiftUI

enum BoardGeometry {
	static let sizeX = 13
	static let count = 247

	static let present: [Bool] = (0..<count).map { CheechSession.boardHolePresent($0) }

	static let base: [CGPoint] = (0..<count).map {
		CGPoint(x: CheechSession.boardHoleX($0), y: CheechSession.boardHoleY($0))
	}

	/// Undirected edges of the lattice, using each hole's forward neighbours
	/// (the same rule as GameBoard::init_neighbors).
	static let edges: [(Int, Int)] = {
		var result: [(Int, Int)] = []
		for i in 0..<count where present[i] {
			let row = i / sizeX
			let candidates: [Int]
			if row % 2 == 0 {
				candidates = [i + 1, i + sizeX + 1, i + sizeX]
			} else {
				candidates = [i + 1, i + sizeX, i + sizeX - 1]
			}
			for n in candidates where n >= 0 && n < count && present[n] {
				// Guard against row-boundary wrap-around: only connect holes
				// that are actually one lattice step apart (as GameHole's
				// set_neighbor distance check does in the core).
				let a = base[i]
				let b = base[n]
				let d = hypot(Double(a.x - b.x), Double(a.y - b.y))
				if d < 1.1 {
					result.append((i, n))
				}
			}
		}
		return result
	}()

	/// Which corner index (1...6) a player occupies, matching GameBoard::START_MAP.
	private static let startMap: [Int: [Int]] = [
		2: [0, 1, 0, 0, 2, 0, 0],
		3: [0, 1, 0, 2, 0, 3, 0],
		4: [0, 0, 1, 2, 0, 3, 4],
		5: [0, 0, 1, 2, 3, 4, 5],
		6: [0, 1, 2, 3, 4, 5, 6],
	]

	static func rotation(numPlayers: Int, player: Int) -> Double {
		guard player >= 1, let map = startMap[numPlayers] else { return 0 }
		for i in 1...6 where map[i] == player {
			return Double(1 - i) * .pi / 3.0
		}
		return 0
	}

	/// Screen positions for every hole, or nil for holes that do not exist.
	static func layout(in size: CGSize, rotation theta: Double) -> [CGPoint?] {
		let cx = 6.5
		let cy = (Double(sizeX - 1) * 0.5) * (sqrt(3.0) / 2.0)

		var transformed = [CGPoint](repeating: .zero, count: count)
		var minX = Double.greatestFiniteMagnitude
		var maxX = -Double.greatestFiniteMagnitude
		var minY = Double.greatestFiniteMagnitude
		var maxY = -Double.greatestFiniteMagnitude

		let c = cos(theta)
		let s = sin(theta)

		for i in 0..<count where present[i] {
			let p = base[i]
			let dx = Double(p.x) - cx
			let dy = Double(p.y) - cy
			let x = cx + dx * c - dy * s
			let y = cy + dx * s + dy * c
			transformed[i] = CGPoint(x: x, y: y)
			minX = min(minX, x); maxX = max(maxX, x)
			minY = min(minY, y); maxY = max(maxY, y)
		}

		let boardW = max(maxX - minX, 0.001)
		let boardH = max(maxY - minY, 0.001)
		// Leave room for the hexagonal board background beyond the star tips.
		let scale = min(Double(size.width) / boardW, Double(size.height) / boardH) * 0.88
		let offsetX = (Double(size.width) - boardW * scale) / 2.0
		let offsetY = (Double(size.height) - boardH * scale) / 2.0

		var result = [CGPoint?](repeating: nil, count: count)
		for i in 0..<count where present[i] {
			let t = transformed[i]
			result[i] = CGPoint(
				x: (Double(t.x) - minX) * scale + offsetX,
				y: (Double(t.y) - minY) * scale + offsetY
			)
		}
		return result
	}

	static func nearestHole(to point: CGPoint, in points: [CGPoint?], maxDistance: CGFloat) -> Int? {
		var best: Int?
		var bestDistance = maxDistance
		for i in 0..<count {
			guard let p = points[i] else { continue }
			let d = hypot(p.x - point.x, p.y - point.y)
			if d < bestDistance {
				bestDistance = d
				best = i
			}
		}
		return best
	}

	/// A rounded hexagonal board slab just outside the six star points.
	static func hexBoardPath(points: [CGPoint?], padding: CGFloat, cornerRadius: CGFloat) -> Path? {
		let hull = convexHull(points.compactMap { $0 })
		guard hull.count == 6 else { return nil }

		let cx = hull.reduce(CGFloat.zero) { $0 + $1.x } / CGFloat(hull.count)
		let cy = hull.reduce(CGFloat.zero) { $0 + $1.y } / CGFloat(hull.count)
		let vertices = hull.map { p -> CGPoint in
			let dx = p.x - cx
			let dy = p.y - cy
			let length = sqrt(dx * dx + dy * dy)
			guard length > 0 else { return p }
			let scale = (length + padding) / length
			return CGPoint(x: cx + dx * scale, y: cy + dy * scale)
		}
		return roundedPolygonPath(vertices, cornerRadius: cornerRadius)
	}

	private static func convexHull(_ points: [CGPoint]) -> [CGPoint] {
		guard points.count > 2 else { return points }
		let sorted = points.sorted {
			if $0.x == $1.x { return $0.y < $1.y }
			return $0.x < $1.x
		}

		func cross(_ o: CGPoint, _ a: CGPoint, _ b: CGPoint) -> CGFloat {
			(a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x)
		}

		var lower: [CGPoint] = []
		for p in sorted {
			while lower.count >= 2 && cross(lower[lower.count - 2], lower[lower.count - 1], p) <= 0 {
				lower.removeLast()
			}
			lower.append(p)
		}

		var upper: [CGPoint] = []
		for p in sorted.reversed() {
			while upper.count >= 2 && cross(upper[upper.count - 2], upper[upper.count - 1], p) <= 0 {
				upper.removeLast()
			}
			upper.append(p)
		}

		lower.removeLast()
		upper.removeLast()
		return lower + upper
	}

	private static func roundedPolygonPath(_ vertices: [CGPoint], cornerRadius: CGFloat) -> Path {
		var path = Path()
		guard vertices.count > 2 else { return path }

		if cornerRadius <= 0 {
			path.move(to: vertices[0])
			for p in vertices.dropFirst() {
				path.addLine(to: p)
			}
			path.closeSubpath()
			return path
		}

		var entries = [CGPoint](repeating: .zero, count: vertices.count)
		var exits = [CGPoint](repeating: .zero, count: vertices.count)

		for i in 0..<vertices.count {
			let prev = vertices[(i + vertices.count - 1) % vertices.count]
			let curr = vertices[i]
			let next = vertices[(i + 1) % vertices.count]
			let toPrev = unitVector(from: curr, to: prev)
			let toNext = unitVector(from: curr, to: next)
			let dot = max(-1, min(1, toPrev.x * toNext.x + toPrev.y * toNext.y))
			let angle = CGFloat(acos(Double(dot)))
			let maxTangent = min(
				hypot(curr.x - prev.x, curr.y - prev.y),
				hypot(curr.x - next.x, curr.y - next.y)
			) * 0.5
			let tangent = min(cornerRadius / CGFloat(tan(Double(angle * 0.5))), maxTangent)
			entries[i] = CGPoint(x: curr.x + toPrev.x * tangent, y: curr.y + toPrev.y * tangent)
			exits[i] = CGPoint(x: curr.x + toNext.x * tangent, y: curr.y + toNext.y * tangent)
		}

		path.move(to: entries[0])
		path.addQuadCurve(to: exits[0], control: vertices[0])
		for i in 1..<vertices.count {
			path.addLine(to: entries[i])
			path.addQuadCurve(to: exits[i], control: vertices[i])
		}
		path.addLine(to: entries[0])
		path.closeSubpath()
		return path
	}

	private static func unitVector(from a: CGPoint, to b: CGPoint) -> CGPoint {
		let dx = b.x - a.x
		let dy = b.y - a.y
		let length = sqrt(dx * dx + dy * dy)
		guard length > 0 else { return .zero }
		return CGPoint(x: dx / length, y: dy / length)
	}
}

enum PegColor {
	static let names = ["", "red", "orange", "yellow", "green", "blue", "purple", "black", "white"]

	static func swiftUIColor(_ color: Int) -> Color {
		guard color >= 1 && color < names.count else { return .gray }
		switch names[color] {
		case "red": return Color(red: 0.80, green: 0.15, blue: 0.15)
		case "orange": return Color(red: 0.95, green: 0.55, blue: 0.10)
		case "yellow": return Color(red: 0.90, green: 0.80, blue: 0.15)
		case "green": return Color(red: 0.20, green: 0.65, blue: 0.25)
		case "blue": return Color(red: 0.15, green: 0.35, blue: 0.80)
		case "purple": return Color(red: 0.55, green: 0.20, blue: 0.70)
		case "black": return Color(red: 0.15, green: 0.15, blue: 0.15)
		case "white": return Color(red: 0.90, green: 0.90, blue: 0.90)
		default: return .gray
		}
	}
}
