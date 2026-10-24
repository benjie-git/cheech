//
//  BoardView.swift
//  Cheech
//
//  Copyright (c) 2005-2026 Ben Levitt
//
//  SPDX-License-Identifier: MIT
//

import SwiftUI

struct BoardView: View {
	@ObservedObject var model: SessionModel
	@ObservedObject var animator: MoveAnimator
	let rotation: Double
	var leadingOverlay: AnyView? = nil
	var trailingOverlay: AnyView? = nil
	var bottomTrailingOverlay: AnyView? = nil

	var body: some View {
		TimelineView(.animation(minimumInterval: 1.0 / 60.0, paused: !animator.isAnimating)) { timeline in
			GeometryReader { geo in
				let points = BoardGeometry.layout(in: geo.size, rotation: rotation)
				let players = model.holes
				let showPegs = model.session.connected && model.session.status != .waiting
				let playerColor = (0...6).map { $0 == 0 ? 0 : Int(model.session.color(forPlayer: $0)) }
				let unit = min(geo.size.width / 14.5, geo.size.height / 18.0)
				let movingTo = animator.destination
				let movingPoint = animator.position(points: points, at: timeline.date)
				let bgRect = boardBackgroundRect(points: points, size: geo.size, unit: unit)
				let backgroundPath = BoardGeometry.hexBoardPath(
					points: points,
					padding: unit * 0.8,
					cornerRadius: unit * 0.8
				)
				let overlayInset = unit * 0.25

				ZStack(alignment: .topLeading) {
					Canvas { context, size in
						// A rounded hexagon just outside the six star points.
						let background = backgroundPath ?? Path(roundedRect: bgRect, cornerRadius: 24)
						context.fill(background, with: .color(Color(red: 0.55, green: 0.40, blue: 0.24)))

						// Lattice
						var lattice = Path()
						for (a, b) in BoardGeometry.edges {
							guard let pa = points[a], let pb = points[b] else { continue }
							lattice.move(to: pa)
							lattice.addLine(to: pb)
						}
						context.stroke(lattice, with: .color(Color(red: 0.49, green: 0.34, blue: 0.20)), lineWidth: max(1, unit * 0.06))

						// Holes
						for i in 0..<BoardGeometry.count where BoardGeometry.present[i] {
							guard let p = points[i] else { continue }
							let r = max(2, unit * 0.46 - 2)
							let rect = CGRect(x: p.x - r, y: p.y - r, width: r * 2, height: r * 2)
							context.fill(Path(ellipseIn: rect), with: .color(Color(red: 0.40, green: 0.26, blue: 0.15)))
						}

						// Pegs (the destination peg is hidden while its move plays)
						for i in 0..<BoardGeometry.count where showPegs && BoardGeometry.present[i] {
							guard let p = points[i], players[i] > 0 else { continue }
							if i == movingTo { continue }
							drawPeg(&context, at: p, color: PegColor.swiftUIColor(playerColor[players[i]]), unit: unit)
						}

						// Selection paths: a yellow polyline through the holes
						// plus a ring on each one (used for the local
						// selection and for other players' telegraphed
						// moves, which share the look).
						func strokeHolePath(_ holes: [Int]) {
							if holes.count > 1 {
								var path = Path()
								if let first = points[holes[0]] { path.move(to: first) }
								for h in holes.dropFirst() {
									if let p = points[h] { path.addLine(to: p) }
								}
								context.stroke(path, with: .color(.yellow), style: StrokeStyle(lineWidth: max(2, unit * 0.12), lineCap: .round, lineJoin: .round))
							}
							for h in Set(holes) {
								guard let p = points[h] else { continue }
								let r = unit * 0.42
								let rect = CGRect(x: p.x - r, y: p.y - r, width: r * 2, height: r * 2)
								context.stroke(Path(ellipseIn: rect), with: .color(.yellow), lineWidth: max(2, unit * 0.10))
							}
						}

						strokeHolePath(model.session.selectedHoles().map { $0.intValue })

						// Another player's in-progress move, telegraphed
						// through the server; same look as a selection.
						strokeHolePath(model.session.telegraphHoles().map { $0.intValue })

						// The peg in flight, drawn on top of everything else.
						if showPegs, let dest = movingTo, let p = movingPoint, players[dest] > 0 {
							drawPeg(&context, at: p, color: PegColor.swiftUIColor(playerColor[players[dest]]), unit: unit)
						}
					}
					.gesture(
						SpatialTapGesture().onEnded { value in
							guard !animator.isAnimating || model.isLocalHumanTurn else { return }
							if let hole = BoardGeometry.nearestHole(to: value.location, in: points, maxDistance: max(26, unit * 0.9)) {
								model.handleTap(hole: hole)
							}
						}
					)

					// Overlays sit at the corners of the board's bounding box,
					// outside the hexagonal slab.
					if let leadingOverlay {
						leadingOverlay
							.padding([.top, .leading], overlayInset)
							.offset(x: bgRect.minX, y: bgRect.minY)
					}
					if let trailingOverlay {
						trailingOverlay
							.padding([.top, .trailing], overlayInset)
							.frame(width: bgRect.width, alignment: .trailing)
							.offset(x: bgRect.minX, y: bgRect.minY)
					}
					if let bottomTrailingOverlay {
						bottomTrailingOverlay
							.padding([.bottom, .trailing], overlayInset)
							.frame(width: bgRect.width, height: bgRect.height, alignment: .bottomTrailing)
							.offset(x: bgRect.minX, y: bgRect.minY)
					}
				}
			}
		}
	}

	// Bounding box of the laid-out holes, padded so the background hugs the
	// lattice instead of filling the whole canvas.
	private func boardBackgroundRect(points: [CGPoint?], size: CGSize, unit: CGFloat) -> CGRect {
		var minX = CGFloat.greatestFiniteMagnitude
		var minY = CGFloat.greatestFiniteMagnitude
		var maxX = -CGFloat.greatestFiniteMagnitude
		var maxY = -CGFloat.greatestFiniteMagnitude
		for p in points.compactMap({ $0 }) {
			minX = min(minX, p.x); maxX = max(maxX, p.x)
			minY = min(minY, p.y); maxY = max(maxY, p.y)
		}
		let pad = unit * 0.8
		let x = max(0, minX - pad)
		let y = max(0, minY - pad)
		return CGRect(
			x: x,
			y: y,
			width: min(size.width, maxX + pad) - x,
			height: min(size.height, maxY + pad) - y
		)
	}

	private func drawPeg(_ context: inout GraphicsContext, at p: CGPoint, color: Color, unit: CGFloat) {
		let r = unit * 0.46
		let rect = CGRect(x: p.x - r, y: p.y - r, width: r * 2, height: r * 2)
		context.fill(Path(ellipseIn: rect.insetBy(dx: r * 0.12, dy: r * 0.12)),
					 with: .color(.white.opacity(0.85)))
		context.fill(Path(ellipseIn: rect), with: .color(color))
		let gloss = CGRect(x: p.x - r * 0.45, y: p.y - r * 0.6,
						   width: r * 0.7, height: r * 0.55)
		context.fill(Path(ellipseIn: gloss), with: .color(.white.opacity(0.35)))
	}
}
