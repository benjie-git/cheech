import Foundation
import CoreGraphics

/// Drives the on-screen replay of a completed move.  The core applies a move
/// in one step, so BoardView asks this object where the moving peg should be
/// drawn right now; each hop is eased in/out and the peg rests at the
/// destination for `done` seconds before the animation ends.
final class MoveAnimator: ObservableObject {
	struct ActiveMove {
		let path: [Int]
		let start: Date
		let step: Double
		let done: Double
	}

	@Published private(set) var active: ActiveMove?
	private var token = 0

	var isAnimating: Bool { active != nil }

	var destination: Int? { active?.path.last }

	func start(path: [Int], step: Double, done: Double, completion: (() -> Void)? = nil) {
		guard path.count > 1 else { return }

		token += 1
		let myToken = token
		active = ActiveMove(path: path, start: Date(), step: step, done: done)

		let total = step * Double(path.count - 1) + done
		DispatchQueue.main.asyncAfter(deadline: .now() + total) { [weak self] in
			guard let self, self.token == myToken else { return }
			self.active = nil
			completion?()
		}
	}

	func stop() {
		token += 1
		active = nil
	}

	/// Location of the moving peg at `date`, or nil when nothing is animating.
	/// `points` is the current (already rotated) board layout.
	func position(points: [CGPoint?], at date: Date) -> CGPoint? {
		guard let move = active, move.path.count > 1 else { return nil }

		let hops = move.path.count - 1
		let step = max(move.step, 0.001)
		let elapsed = max(date.timeIntervalSince(move.start), 0)
		let hop = min(Int(elapsed / step), hops - 1)
		let local = min(max((elapsed - Double(hop) * step) / step, 0), 1)

		// Cubic ease-in-out.
		let eased = local < 0.5 ? 2 * local * local : 1 - 2 * (1 - local) * (1 - local)

		guard let a = points[move.path[hop]], let b = points[move.path[hop + 1]] else {
			return nil
		}
		return CGPoint(x: a.x + (b.x - a.x) * eased, y: a.y + (b.y - a.y) * eased)
	}
}
