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
		let scale = min(Double(size.width) / boardW, Double(size.height) / boardH) * 0.94
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
		case "white": return Color(red: 0.95, green: 0.95, blue: 0.95)
		default: return .gray
		}
	}
}
