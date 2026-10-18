export const SIZE_X = 13;
export const SIZE_Y = 19;
export const HOLE_COUNT = SIZE_X * SIZE_Y;

export const COLOR_NAMES = [
	"",
	"Red",
	"Orange",
	"Yellow",
	"Green",
	"Blue",
	"Purple",
	"Black",
	"White",
];

export const PEG_FILL = [
	"#664226",
	"#e20000",
	"#fc8e00",
	"#f6f300",
	"#00d600",
	"#0095e2",
	"#b500e2",
	"#202020",
	"#f7f7f7",
];

const PEG_EDGE = [
	"#664226",
	"#702323",
	"#b66700",
	"#6e641e",
	"#008c00",
	"#235671",
	"#760094",
	"#1a1a1a",
	"#bfbfbf",
];

const WOOD = "#8c663d";
const LATTICE = "#7d5733";
const HOLE = "#664226";
const SELECTION = "#ffe600";

const SQRT3_2 = Math.sqrt(3) / 2;

const BASE = [];
for (let i = 0; i < HOLE_COUNT; i++) {
	const col = i % SIZE_X;
	const row = Math.floor(i / SIZE_X);
	BASE.push({
		x: col + (row % 2 === 0 ? 0.5 : 0),
		y: row * SQRT3_2,
	});
}

const EDGES = [];
for (let i = 0; i < HOLE_COUNT; i++) {
	const row = Math.floor(i / SIZE_X);
	const candidates =
		row % 2 === 0
			? [i + 1, i + SIZE_X + 1, i + SIZE_X]
			: [i + 1, i + SIZE_X, i + SIZE_X - 1];

	for (const n of candidates) {
		if (n < 0 || n >= HOLE_COUNT) continue;
		const a = BASE[i];
		const b = BASE[n];
		if (Math.hypot(a.x - b.x, a.y - b.y) < 1.1) EDGES.push([i, n]);
	}
}

function easeInOut(t) {
	return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
}

function clamp01(value) {
	return Math.max(0, Math.min(1, value));
}

function roundedRect(ctx, x, y, w, h, radius) {
	const r = Math.max(0, Math.min(radius, w / 2, h / 2));
	ctx.beginPath();
	ctx.moveTo(x + r, y);
	ctx.arcTo(x + w, y, x + w, y + h, r);
	ctx.arcTo(x + w, y + h, x, y + h, r);
	ctx.arcTo(x, y + h, x, y, r);
	ctx.arcTo(x, y, x + w, y, r);
	ctx.closePath();
}

function moveDuration(holes) {
	return Math.max(220, Math.min(700, 170 + Math.max(0, holes - 1) * 80));
}

export class BoardView {
	constructor(canvas) {
		this.canvas = canvas;
		this.ctx = canvas.getContext("2d");
		this.board = new Uint8Array(HOLE_COUNT).fill(9);
		this.players = new Uint8Array(7);
		this.hasBoard = false;
		this.selection = [];
		this.telegraph = [];
		this.rotation = 0;
		this.rotationTarget = 0;
		this.rotationReady = false;
		this.onMoveEnd = null;

		this.width = 0;
		this.height = 0;
		this.dpr = 1;
		this.points = new Array(HOLE_COUNT).fill(null);
		this.unit = 1;
		this.bgRect = null;

		this._rotation = null;
		this._move = null;
		this._raf = 0;

		this._resizeObserver = new ResizeObserver(() => this._resize());
		this._resizeObserver.observe(canvas.parentElement || canvas);
		this._resize();
	}

	reset() {
		this.board.fill(9);
		this.players.fill(0);
		this.hasBoard = false;
		this.selection = [];
		this.telegraph = [];
		this.rotation = 0;
		this.rotationTarget = 0;
		this.rotationReady = false;
		this._rotation = null;
		this._move = null;
		this._requestDraw();
	}

	setBoard(values) {
		const next = new Uint8Array(HOLE_COUNT).fill(9);
		const count = Math.min(HOLE_COUNT, values.length);
		for (let i = 0; i < count; i++) {
			const value = values[i];
			next[i] = value >= 0 && value <= 9 ? value : 9;
		}
		this.board = next;
		this.hasBoard = true;
		this.selection = [];
		this.telegraph = [];
		this._move = null;
		this._requestDraw();
	}

	setPlayerColor(posn, color) {
		if (posn >= 1 && posn <= 6) {
			this.players[posn] = color >= 0 && color <= 8 ? color : 0;
			this._requestDraw();
		}
	}

	setSelection(holes) {
		this.selection = Array.isArray(holes) ? holes.slice() : [];
		this._requestDraw();
	}

	setTelegraph(holes) {
		this.telegraph = Array.isArray(holes) ? holes.slice() : [];
		this._requestDraw();
	}

	setRotation(value) {
		const target =
			value >= 1 && value <= 6 ? (1 - value) * Math.PI / 3 : 0;

		if (!this.rotationReady) {
			this.rotation = target;
			this.rotationTarget = target;
			this.rotationReady = true;
			this._rotation = null;
			this._requestDraw();
			return;
		}

		let delta = target - this.rotation;
		delta = ((delta + Math.PI) % (2 * Math.PI) + 2 * Math.PI) % (2 * Math.PI) - Math.PI;

		if (Math.abs(delta) < 0.0001) {
			this.rotation = target;
			this.rotationTarget = target;
			this._rotation = null;
			this._requestDraw();
			return;
		}

		this.rotationTarget = target;
		this._rotation = {
			from: this.rotation,
			delta,
			start: performance.now(),
			duration: 420,
		};
		this._requestDraw();
	}

	applyMove(holes, { animate = true } = {}) {
		if (!Array.isArray(holes) || holes.length < 2) return;
		const from = holes[0];
		const to = holes[holes.length - 1];
		if (from < 0 || from >= HOLE_COUNT || to < 0 || to >= HOLE_COUNT) return;

		const player = this.board[from];
		if (!player || player === 9) return;

		this.board[from] = 0;
		this.board[to] = player;
		this.selection = [];
		this.telegraph = [];

		if (animate && this.hasBoard) {
			this._move = {
				holes: holes.slice(),
				player,
				start: performance.now(),
				duration: moveDuration(holes.length),
			};
		} else {
			this._move = null;
		}
		this._requestDraw();
	}

	resize() {
		this._resize();
	}

	isMoveAnimating() {
		return Boolean(this._move);
	}

	isRotating() {
		return Boolean(this._rotation);
	}

	holeAt(x, y) {
		if (!this.hasBoard || this._rotation) return -1;

		const maxDistance = Math.max(24, this.unit * 0.9);
		let best = -1;
		let bestDistance = maxDistance;

		for (let i = 0; i < HOLE_COUNT; i++) {
			if (this.board[i] === 9) continue;
			const p = this.points[i];
			if (!p) continue;
			const distance = Math.hypot(p.x - x, p.y - y);
			if (distance < bestDistance) {
				bestDistance = distance;
				best = i;
			}
		}

		return best;
	}

	_resize() {
		const rect = this.canvas.getBoundingClientRect();
		const dpr = Math.min(window.devicePixelRatio || 1, 2.5);
		const width = Math.max(1, Math.round(rect.width));
		const height = Math.max(1, Math.round(rect.height));

		if (width !== this.width || height !== this.height || dpr !== this.dpr) {
			this.width = width;
			this.height = height;
			this.dpr = dpr;
			this.canvas.width = Math.round(width * dpr);
			this.canvas.height = Math.round(height * dpr);
		}
		this._requestDraw();
	}

	_requestDraw() {
		if (this._raf) return;
		this._raf = requestAnimationFrame(() => {
			this._raf = 0;
			this.draw();
			if (this._rotation || this._move) this._requestDraw();
		});
	}

	_layout(theta) {
		const cx = 6.5;
		const cy = ((SIZE_X - 1) * 0.5) * SQRT3_2;
		const cos = Math.cos(theta);
		const sin = Math.sin(theta);

		let minX = Infinity;
		let minY = Infinity;
		let maxX = -Infinity;
		let maxY = -Infinity;
		const transformed = new Array(HOLE_COUNT).fill(null);

		for (let i = 0; i < HOLE_COUNT; i++) {
			if (this.board[i] === 9) continue;
			const p = BASE[i];
			const dx = p.x - cx;
			const dy = p.y - cy;
			const x = cx + dx * cos - dy * sin;
			const y = cy + dx * sin + dy * cos;
			transformed[i] = { x, y };
			if (x < minX) minX = x;
			if (x > maxX) maxX = x;
			if (y < minY) minY = y;
			if (y > maxY) maxY = y;
		}

		if (!Number.isFinite(minX)) {
			this.points = new Array(HOLE_COUNT).fill(null);
			this.unit = 1;
			this.bgRect = null;
			return;
		}

		const boardW = Math.max(maxX - minX, 0.001);
		const boardH = Math.max(maxY - minY, 0.001);
		const pad = 0.8;
		const scale =
			Math.min(
				this.width / (boardW + pad * 2),
				this.height / (boardH + pad * 2)
			) * 0.96;

		const offsetX = (this.width - boardW * scale) / 2;
		const offsetY = (this.height - boardH * scale) / 2;

		const points = new Array(HOLE_COUNT).fill(null);
		for (let i = 0; i < HOLE_COUNT; i++) {
			const p = transformed[i];
			if (!p) continue;
			points[i] = {
				x: (p.x - minX) * scale + offsetX,
				y: (p.y - minY) * scale + offsetY,
			};
		}

		this.points = points;
		this.unit = scale;
		this.bgRect = {
			x: offsetX - pad * scale,
			y: offsetY - pad * scale,
			w: boardW * scale + pad * 2 * scale,
			h: boardH * scale + pad * 2 * scale,
		};
	}

	draw() {
		const ctx = this.ctx;
		ctx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
		ctx.clearRect(0, 0, this.width, this.height);

		const now = performance.now();

		if (this._rotation) {
			const t = clamp01((now - this._rotation.start) / this._rotation.duration);
			this.rotation =
				this._rotation.from + this._rotation.delta * easeInOut(t);
			if (t >= 1) {
				this.rotation = this.rotationTarget;
				this._rotation = null;
			}
		}

		let movingTo = -1;
		let movingPoint = null;
		let movingPlayer = 0;

		if (this._move) {
			const t = clamp01((now - this._move.start) / this._move.duration);
			const eased = easeInOut(t);
			const path = this._move.holes;
			const segments = Math.max(1, path.length - 1);
			const f = eased * segments;
			const index = Math.min(segments - 1, Math.floor(f));
			const local = f - index;
			const a = this.points[path[index]];
			const b = this.points[path[index + 1]];

			if (a && b) {
				movingPoint = {
					x: a.x + (b.x - a.x) * local,
					y: a.y + (b.y - a.y) * local,
				};
			}
			movingTo = path[path.length - 1];
			movingPlayer = this._move.player;

			if (t >= 1) {
				this._move = null;
				movingPoint = null;
				movingTo = -1;
				if (this.onMoveEnd) this.onMoveEnd();
			}
		}

		if (!this.hasBoard) {
			ctx.fillStyle = "rgba(255, 255, 255, .55)";
			ctx.font = `600 ${Math.max(13, Math.min(20, this.width / 32))}px -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif`;
			ctx.textAlign = "center";
			ctx.textBaseline = "middle";
			ctx.fillText("Connecting to the board…", this.width / 2, this.height / 2);
			return;
		}

		this._layout(this.rotation);

		if (this.bgRect) {
			roundedRect(
				ctx,
				this.bgRect.x,
				this.bgRect.y,
				this.bgRect.w,
				this.bgRect.h,
				24
			);
			ctx.fillStyle = WOOD;
			ctx.fill();
		}

		const unit = this.unit;
		ctx.strokeStyle = LATTICE;
		ctx.lineWidth = Math.max(1, unit * 0.06);
		ctx.lineCap = "round";
		ctx.lineJoin = "round";

		ctx.beginPath();
		for (const [a, b] of EDGES) {
			if (this.board[a] === 9 || this.board[b] === 9) continue;
			const pa = this.points[a];
			const pb = this.points[b];
			if (!pa || !pb) continue;
			ctx.moveTo(pa.x, pa.y);
			ctx.lineTo(pb.x, pb.y);
		}
		ctx.stroke();

		const holeRadius = Math.max(2, unit * 0.46 - 2);
		ctx.fillStyle = HOLE;
		for (let i = 0; i < HOLE_COUNT; i++) {
			if (this.board[i] === 9) continue;
			const p = this.points[i];
			if (!p) continue;
			ctx.beginPath();
			ctx.arc(p.x, p.y, holeRadius, 0, Math.PI * 2);
			ctx.fill();
		}

		for (let i = 0; i < HOLE_COUNT; i++) {
			const player = this.board[i];
			if (!player || player === 9) continue;
			if (i === movingTo) continue;
			const p = this.points[i];
			if (!p) continue;
			this._drawPeg(ctx, p.x, p.y, this.players[player], unit);
		}

		this._drawPath(ctx, this.selection, unit);
		this._drawPath(ctx, this.telegraph, unit);

		if (this._move && movingPoint) {
			this._drawPeg(ctx, movingPoint.x, movingPoint.y, this.players[movingPlayer], unit);
		}
	}

	_drawPeg(ctx, x, y, color, unit) {
		const r = Math.max(3, unit * 0.46);
		const edge = color ? PEG_EDGE[color] : "#555";
		const fill = color ? PEG_FILL[color] : "#888";

		ctx.beginPath();
		ctx.arc(x, y, r, 0, Math.PI * 2);
		ctx.fillStyle = edge;
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x, y, r * 0.88, 0, Math.PI * 2);
		ctx.fillStyle = fill;
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x - r * 0.28, y - r * 0.34, r * 0.3, 0, Math.PI * 2);
		ctx.fillStyle = "rgba(255, 255, 255, .35)";
		ctx.fill();
	}

	_drawPath(ctx, holes, unit) {
		if (!holes.length) return;

		ctx.strokeStyle = SELECTION;
		ctx.lineCap = "round";
		ctx.lineJoin = "round";

		if (holes.length > 1) {
			ctx.beginPath();
			let started = false;
			for (const hole of holes) {
				const p = this.points[hole];
				if (!p) continue;
				if (!started) {
					ctx.moveTo(p.x, p.y);
					started = true;
				} else {
					ctx.lineTo(p.x, p.y);
				}
			}
			ctx.lineWidth = Math.max(2, unit * 0.12);
			ctx.stroke();
		}

		ctx.lineWidth = Math.max(2, unit * 0.1);
		const seen = new Set();
		for (const hole of holes) {
			if (seen.has(hole)) continue;
			seen.add(hole);
			const p = this.points[hole];
			if (!p) continue;
			ctx.beginPath();
			ctx.arc(p.x, p.y, unit * 0.42, 0, Math.PI * 2);
			ctx.stroke();
		}
	}
}
