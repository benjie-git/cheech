import { CheechConnection, cleanName } from "./protocol.js";
import { BoardView, PEG_FILL, COLOR_NAMES } from "./board.js";

const STATUS = { WAITING: 0, START: 1, PLAYING: 2, WON: 3, END: 4 };
const STORAGE_KEY = "cheech.prefs.v1";
const REDUCED_MOTION = window.matchMedia
	? window.matchMedia("(prefers-reduced-motion: reduce)")
	: null;

const $ = (id) => document.getElementById(id);
const els = {
	app: $("app"),
	gameMenu: $("gameMenu"),
	playerMenu: $("playerMenu"),
	helpMenu: $("helpMenu"),
	connPill: $("connPill"),
	connText: $("connText"),
	joinScreen: $("joinScreen"),
	joinForm: $("joinForm"),
	joinName: $("joinName"),
	joinColors: $("joinColors"),
	joinSpectator: $("joinSpectator"),
	joinError: $("joinError"),
	joinButton: $("joinButton"),
	gameScreen: $("gameScreen"),
	boardStage: $("boardStage"),
	board: $("board"),
	turnBadge: $("turnBadge"),
	turnDot: $("turnDot"),
	turnText: $("turnText"),
	moveBadge: $("moveBadge"),
	winnerBanner: $("winnerBanner"),
	winnerText: $("winnerText"),
	moveControls: $("moveControls"),
	cancelMove: $("cancelMove"),
	commitMove: $("commitMove"),
	boardHint: $("boardHint"),
	playerList: $("playerList"),
	spectatorChip: $("spectatorChip"),
	gameInfo: $("gameInfo"),
	chatLog: $("chatLog"),
	chatForm: $("chatForm"),
	chatInput: $("chatInput"),
	toast: $("toast"),
	modal: $("modal"),
	modalTitle: $("modalTitle"),
	modalBody: $("modalBody"),
	modalActions: $("modalActions"),
	modalClose: $("modalClose"),
	changeColorAction: $("changeColorAction"),
};

const CONNECTION_LABELS = {
	idle: "Not connected",
	connecting: "Connecting…",
	online: "Connected",
	offline: "Reconnecting…",
	reconnecting: "Reconnecting…",
	lost: "Connection lost",
	exited: "Not connected",
};

function readPrefs() {
	try {
		const raw = localStorage.getItem(STORAGE_KEY);
		if (!raw) return { name: "", color: 1 };
		const value = JSON.parse(raw);
		const color = Number.isInteger(value.color) && value.color >= 1 && value.color <= 8
			? value.color : 1;
		return { name: cleanName(value.name || ""), color };
	} catch (_error) {
		return { name: "", color: 1 };
	}
}

function savePrefs(name, color) {
	try {
		localStorage.setItem(STORAGE_KEY, JSON.stringify({
			name: cleanName(name || ""),
			color: Number.isInteger(color) && color >= 1 && color <= 8 ? color : 1,
		}));
	} catch (_error) {
		// Private browsing and full storage are not fatal.
	}
}

const prefs = readPrefs();
let joinColor = prefs.color;
let modalDismissible = true;
let lostShown = false;
let toastTimer = 0;
let toastHideTimer = 0;

const state = {
	myPosn: 0,
	spectator: false,
	myName: "",
	myColor: 0,
	joined: false,
	players: new Map(),
	numPlayers: 0,
	hasInfo: false,
	hasBoard: false,
	rules: { longs: false, hops: false, stops: false },
	status: STATUS.WAITING,
	currentPlayer: 0,
	moveCount: 1,
	selection: [],
	telegraph: [],
	animatingPlayer: 0,
};

const boardView = new BoardView(els.board);
boardView.onMoveEnd = () => {
	state.animatingPlayer = 0;
	renderTurn();
	renderPlayers();
	updateMoveControls();
};

const conn = new CheechConnection();
conn.onCommand = handleCommand;
conn.onStatus = handleStatus;

const menus = Array.from(document.querySelectorAll("details.menu"));

function prefersReducedMotion() {
	return Boolean(REDUCED_MOTION && REDUCED_MOTION.matches);
}

function el(tag, attrs = {}, ...children) {
	const node = document.createElement(tag);

	for (const [key, value] of Object.entries(attrs || {})) {
		if (value == null || value === false) continue;

		if (key === "class") {
			node.className = value;
		} else if (key === "text") {
			node.textContent = value;
		} else if (key === "html") {
			node.innerHTML = value;
		} else if (key === "dataset") {
			Object.assign(node.dataset, value);
		} else if (key === "style" && typeof value === "object") {
			for (const [property, propertyValue] of Object.entries(value)) {
				if (propertyValue == null) continue;
				if (property.startsWith("--")) {
					node.style.setProperty(property, propertyValue);
				} else {
					node.style[property] = propertyValue;
				}
			}
		} else if (key.startsWith("on") && typeof value === "function") {
			node.addEventListener(key.slice(2).toLowerCase(), value);
		} else if (["checked", "disabled", "hidden", "selected", "open", "required", "readOnly"].includes(key)) {
			node[key] = Boolean(value);
		} else {
			node.setAttribute(key, value);
		}
	}

	for (const child of children.flat(Infinity)) {
		if (child == null || child === false) continue;
		node.append(child instanceof Node ? child : document.createTextNode(String(child)));
	}

	return node;
}

function modalButton(label, options = {}) {
	const button = el("button", {
		class: `btn${options.primary ? " primary" : ""}${options.danger ? " danger" : ""}`,
		type: options.submit ? "submit" : "button",
		form: options.form,
		disabled: options.disabled,
		text: label,
	});
	if (options.onClick) button.addEventListener("click", options.onClick);
	return button;
}

function pegDot(color) {
	const dot = el("span", { class: "peg-dot" });
	if (Number.isInteger(color) && color >= 1 && color <= 8) {
		dot.style.background = PEG_FILL[color];
	} else {
		dot.classList.add("empty");
	}
	return dot;
}

function colorSwatches(container, selected, onPick) {
	container.replaceChildren();
	for (let color = 1; color <= 8; color++) {
		const button = el("button", {
			class: "swatch",
			type: "button",
			role: "radio",
			"aria-checked": color === selected ? "true" : "false",
			"aria-label": COLOR_NAMES[color] || `Color ${color}`,
			title: COLOR_NAMES[color] || `Color ${color}`,
			style: { background: PEG_FILL[color] },
		});
		button.addEventListener("click", () => {
			selected = color;
			for (const item of container.children) {
				item.setAttribute("aria-checked", item === button ? "true" : "false");
			}
			onPick?.(color);
		});
		container.append(button);
	}
}

function showJoinError(message) {
	els.joinError.textContent = message;
	els.joinError.hidden = false;
}

function hideJoinError() {
	els.joinError.textContent = "";
	els.joinError.hidden = true;
}

function playerName(posn) {
	const player = state.players.get(posn);
	return (player && player.name) || `Player ${posn}`;
}

function colorName(color) {
	return COLOR_NAMES[color] || "Unassigned";
}

function canChangeIdentity() {
	return state.joined && state.myPosn > 0 && !state.spectator;
}

function canUseHistory() {
	return state.joined
		&& !state.spectator
		&& (state.status === STATUS.START
			|| state.status === STATUS.PLAYING
			|| state.status === STATUS.WON
			|| state.status === STATUS.END);
}

function canPlay() {
	return state.joined
		&& !state.spectator
		&& state.myPosn > 0
		&& state.hasBoard
		&& state.currentPlayer === state.myPosn
		&& !allPlayersFinished();
}

// The server reports "Won" as soon as the first player finishes.  The other
// clients keep the remaining players on the board until everyone is home, so
// only treat the game as over once every seat has a finish score.
function allPlayersFinished() {
	const count = state.numPlayers;
	if (!count || count < 1) return false;

	for (let posn = 1; posn <= count; posn++) {
		const player = state.players.get(posn);
		if (!player || !(player.finishedMoves > 0)) return false;
	}
	return true;
}

function renderJoinColors() {
	colorSwatches(els.joinColors, joinColor, (color) => {
		joinColor = color;
	});
}

function showGameView() {
	els.app.dataset.view = "game";
	els.joinScreen.hidden = true;
	els.gameScreen.hidden = false;
	requestAnimationFrame(() => boardView.resize());
}

function showJoinView({ error = "", message = "" } = {}) {
	state.joined = false;
	lostShown = false;
	resetSessionState();

	els.app.dataset.view = "join";
	els.gameScreen.hidden = true;
	els.joinScreen.hidden = false;
	els.gameMenu.hidden = true;
	els.playerMenu.hidden = true;

	els.joinName.value = state.myName || prefs.name;
	joinColor = state.myColor || prefs.color || 1;
	els.joinSpectator.checked = state.spectator;
	renderJoinColors();

	els.joinButton.disabled = false;
	els.joinButton.textContent = "Join Game";

	if (error) showJoinError(error);
	else hideJoinError();

	updateChrome();
	if (message) toast(message);
}

function resetSessionState() {
	state.myPosn = 0;
	state.players.clear();
	state.numPlayers = 0;
	state.hasInfo = false;
	state.hasBoard = false;
	state.rules = { longs: false, hops: false, stops: false };
	state.status = STATUS.WAITING;
	state.currentPlayer = 0;
	state.moveCount = 1;
	state.selection = [];
	state.telegraph = [];
	state.animatingPlayer = 0;

	boardView.reset();
	clearChat();
	renderPlayers();
	renderTurn();
	renderGameInfo();
	renderWinner();
	updateMoveControls();
	updateChrome();
}

function beginSession() {
	state.joined = false;
	lostShown = false;
	resetSessionState();
	showGameView();

	state.myName = state.myName || cleanName(els.joinName.value) || "Player";
	state.myColor = state.myColor || joinColor || 1;
	state.myName = cleanName(state.myName) || "Player";

	els.joinButton.disabled = true;
	els.joinButton.textContent = "Joining…";
	updateChrome();

	conn.connect({
		name: state.myName,
		color: state.myColor,
		spectator: state.spectator,
	});
}

function updateChrome() {
	els.gameMenu.hidden = !state.joined;
	els.playerMenu.hidden = !(state.joined && state.myPosn > 0 && !state.spectator);
	els.spectatorChip.hidden = !state.spectator;

	const canChange = canChangeIdentity();
	const nameAction = els.playerMenu.querySelector('[data-action="name"]');
	if (nameAction) nameAction.disabled = !canChange;
	els.changeColorAction.disabled = !canChange;

	const history = canUseHistory();
	const undo = els.gameMenu.querySelector('[data-action="undo"]');
	const redo = els.gameMenu.querySelector('[data-action="redo"]');
	if (undo) undo.disabled = !history;
	if (redo) redo.disabled = !history;
}

function handleStatus(status) {
	const label = CONNECTION_LABELS[status] || "Not connected";
	els.connPill.dataset.state = status;
	els.connText.textContent = label;

	if (status === "online") {
		lostShown = false;
		return;
	}

	if (status !== "lost") return;

	if (state.joined) {
		if (!lostShown) {
			lostShown = true;
			showLostDialog();
		}
	} else if (els.app.dataset.view === "join") {
		showJoinError("Could not connect to the board. Check that the server is still running.");
	} else {
		showJoinView({
			error: "Could not connect to the board. Check that the server is still running.",
		});
	}
}

function handleCommand(name, args) {
	switch (name) {
		case "id":
			state.joined = true;
			updateChrome();
			break;

		case "setposn": {
			const posn = Number.parseInt(args, 10);
			if (posn >= 1 && posn <= 6) {
				state.myPosn = posn;
				state.spectator = false;
			}
			updateChrome();
			renderPlayers();
			updateMoveControls();
			break;
		}

		case "add": {
			const match = /^(\d+)\s+(\d+)\s*(.*)$/.exec(args);
			if (!match) break;
			const posn = Number.parseInt(match[1], 10);
			const color = Number.parseInt(match[2], 10);
			const name = match[3].trim();
			if (posn < 1 || posn > 6) break;
			const previous = state.players.get(posn);
			state.players.set(posn, {
				posn,
				color,
				name,
				finishedMoves: previous?.finishedMoves ?? 0,
			});
			boardView.setPlayerColor(posn, color);
			if (posn === state.myPosn) {
				if (name) state.myName = name;
				if (color > 0) {
					state.myColor = color;
					joinColor = color;
					savePrefs(state.myName, color);
				}
			}
			renderPlayers();
			renderTurn();
			updateChrome();
			break;
		}

		case "remove": {
			const posn = Number.parseInt(args, 10);
			if (posn >= 1 && posn <= 6) {
				state.players.delete(posn);
				if (posn === state.myPosn) state.myPosn = 0;
			}
			renderPlayers();
			renderTurn();
			updateChrome();
			updateMoveControls();
			break;
		}

		case "board": {
			const values = args.trim().split(/\s+/).map((value) => Number.parseInt(value, 10));
			if (values.length !== 247) break;
			state.animatingPlayer = 0;
			state.selection = [];
			state.telegraph = [];
			for (const player of state.players.values()) player.finishedMoves = 0;
			boardView.setBoard(values);
			state.hasBoard = true;
			renderPlayers();
			renderTurn();
			renderWinner();
			updateMoveControls();
			updateChrome();
			break;
		}

		case "rotate": {
			const rotation = Number.parseInt(args, 10);
			if (rotation >= 0 && rotation <= 6) boardView.setRotation(rotation);
			break;
		}

		case "info": {
			const parts = args.trim().split(/\s+/);
			const numPlayers = Number.parseInt(parts[0], 10);
			if (numPlayers >= 0 && numPlayers <= 6) state.numPlayers = numPlayers;
			state.rules = {
				longs: parts[1] === "1",
				hops: parts[2] === "1",
				stops: parts[3] === "1",
			};
			state.hasInfo = true;
			renderPlayers();
			renderGameInfo();
			renderTurn();
			updateChrome();
			break;
		}

		case "turn": {
			const parts = args.trim().split(/\s+/);
			const posn = Number.parseInt(parts[0], 10);
			const status = Number.parseInt(parts[1], 10);
			const moveCount = Number.parseInt(parts[2], 10);
			if (Number.isFinite(posn)) state.currentPlayer = posn;
			if (status >= 0 && status <= 4) state.status = status;
			if (Number.isFinite(moveCount)) state.moveCount = moveCount;
			if (!boardView.isMoveAnimating()) state.animatingPlayer = 0;
			clearSelection();
			renderTurn();
			renderPlayers();
			renderWinner();
			updateMoveControls();
			updateChrome();
			break;
		}

		case "finish": {
			const parts = args.trim().split(/\s+/);
			const posn = Number.parseInt(parts[0], 10);
			const moves = Number.parseInt(parts[1], 10);
			if (posn < 1 || posn > 6) break;
			let player = state.players.get(posn);
			if (!player) {
				player = { posn, color: 0, name: `Player ${posn}`, finishedMoves: 0 };
				state.players.set(posn, player);
			}
			player.finishedMoves = Number.isFinite(moves) ? moves : 0;
			renderPlayers();
			renderWinner();
			break;
		}

		case "chat":
			appendChat(args);
			break;

		case "show": {
			const parts = args.trim().split(/\s+/);
			const count = Number.parseInt(parts[0], 10);
			if (!Number.isFinite(count) || count <= 0) {
				clearSelection();
				break;
			}
			const holes = parts.slice(1, 1 + count).map((value) => Number.parseInt(value, 10));
			const own = state.myPosn > 0
				&& holes.length > 0
				&& boardView.board[holes[0]] === state.myPosn;
			if (own) {
				state.selection = holes;
				state.telegraph = [];
			} else {
				state.selection = [];
				state.telegraph = holes;
			}
			boardView.setSelection(state.selection);
			boardView.setTelegraph(state.telegraph);
			updateMoveControls();
			break;
		}

		case "hide":
			clearSelection();
			break;

		case "choose_name":
			openNameDialog({ forced: true, suggested: args.trim() });
			break;

		case "choose_color":
			openColorDialog({ forced: true });
			break;

		case "move": {
			const parts = args.trim().split(/\s+/);
			const count = Number.parseInt(parts[0], 10);
			if (!Number.isFinite(count) || count < 2) break;
			const holes = parts.slice(1, 1 + count).map((value) => Number.parseInt(value, 10));
			if (holes.length < 2 || holes.some((hole) => !Number.isFinite(hole))) break;
			const mover = boardView.board[holes[0]];
			if (mover < 1 || mover > 6) break;

			// A human's path is already visible from their show messages, and
			// the local player's move should snap so a bot reply cannot catch
			// it mid-flight.  Animate other moves only when the committed path
			// was not telegraphed, matching the desktop and iOS clients.
			const telegraphed = pathsEqual(state.telegraph, holes)
				|| pathsEqual(state.selection, holes);
			const animate = !prefersReducedMotion()
				&& mover !== state.myPosn
				&& !telegraphed;

			state.animatingPlayer = animate ? mover : 0;
			clearSelection();
			boardView.applyMove(holes, { animate });
			renderTurn();
			renderPlayers();
			updateMoveControls();
			break;
		}

		case "ping":
			break;

		case "exit":
			handleExit();
			break;
	}
}

function handleExit() {
	const wasJoined = state.joined;
	state.joined = false;
	closeModal();
	showJoinView({ message: wasJoined ? "You left the board." : "" });
}

function renderPlayers() {
	let count = state.numPlayers;
	if (!count) {
		count = 0;
		for (const posn of state.players.keys()) count = Math.max(count, posn);
	}
	count = Math.min(6, Math.max(0, count));

	els.playerList.replaceChildren();

	if (count === 0) {
		els.playerList.append(el("li", { class: "chat-empty", text: "Waiting for players…" }));
		return;
	}

	const displayed = state.animatingPlayer || state.currentPlayer;
	const finished = allPlayersFinished();

	for (let posn = 1; posn <= count; posn++) {
		const player = state.players.get(posn);
		const open = !player;
		const current = Boolean(player) && !finished && posn === displayed;
		const row = el("li", {
			class: `player-row${current ? " current" : ""}${open ? " open" : ""}`,
		});

		row.append(pegDot(open ? 0 : player.color));

		const main = el("span", { class: "player-main" });
		main.append(el("span", {
			class: "player-name",
			text: open ? "Open seat" : (player.name || `Player ${posn}`),
		}));
		if (player && player.finishedMoves > 0) {
			main.append(el("span", {
				class: "player-moves",
				text: `(${player.finishedMoves})`,
			}));
		}
		if (posn === state.myPosn) {
			main.append(el("span", { class: "you-chip", text: "You" }));
		}
		row.append(main);

		row.title = open
			? `Seat ${posn} is open`
			: `${player.name || `Player ${posn}`} — ${colorName(player.color)}`
				+ (player.finishedMoves > 0 ? `, finished in ${player.finishedMoves} moves` : "");
		if (current) row.setAttribute("aria-current", "true");

		els.playerList.append(row);
	}
}

function renderTurn() {
	const displayed = state.animatingPlayer || state.currentPlayer;
	const finished = allPlayersFinished();
	let text = "Waiting…";
	let myTurn = false;
	let showDot = false;

	if (state.status === STATUS.WAITING) {
		const present = state.players.size;
		text = state.numPlayers > 0
			? `Waiting for players (${present}/${state.numPlayers})`
			: "Waiting for players";
	} else if (finished) {
		text = "Game over";
	} else if (displayed > 0) {
		showDot = true;
		myTurn = displayed === state.myPosn && state.myPosn > 0 && !state.spectator;
		text = myTurn ? "Your move" : `${playerName(displayed)}’s move`;
	} else if (state.status === STATUS.START) {
		text = "Starting…";
	}

	els.turnText.textContent = text;
	els.turnBadge.classList.toggle("my-turn", myTurn);

	const color = displayed > 0 ? state.players.get(displayed)?.color : 0;
	els.turnDot.hidden = !showDot;
	els.turnDot.style.background = color >= 1 && color <= 8 ? PEG_FILL[color] : "#8a9096";
	els.moveBadge.textContent = `Move ${Math.max(1, state.moveCount)}`;
}

function renderGameInfo() {
	els.gameInfo.replaceChildren();

	if (!state.hasInfo) {
		els.gameInfo.append(el("div", { class: "info-row" },
			el("dt", { text: "Game" }),
			el("dd", { text: "Connecting…" })));
	} else {
		const count = state.numPlayers;
		els.gameInfo.append(
			el("div", { class: "info-row" },
				el("dt", { text: "Players" }),
				el("dd", { text: `${count} player${count === 1 ? "" : "s"}` })),
			el("div", { class: "info-row" },
				el("dt", { text: "Long jumps" }),
				el("dd", { text: state.rules.longs ? "Allowed" : "Off" })),
			el("div", { class: "info-row" },
				el("dt", { text: "Opponents’ goals" }),
				el("dd", { text: state.rules.hops ? "Pass through" : "Blocked" })),
			el("div", { class: "info-row" },
				el("dt", { text: "Stop in goals" }),
				el("dd", { text: state.rules.stops ? "Allowed" : "Not allowed" })),
		);
	}

	const card = els.gameInfo.closest(".card");
	if (card && !card.querySelector(".card-note")) {
		card.append(el("p", {
			class: "card-note",
			text: "The host controls game settings.",
		}));
	}
}

function renderWinner() {
	if (!allPlayersFinished()) {
		els.winnerBanner.hidden = true;
		return;
	}

	const finished = Array.from(state.players.values())
		.filter((player) => player.finishedMoves > 0)
		.sort((a, b) => a.finishedMoves - b.finishedMoves || a.posn - b.posn);

	if (finished.length) {
		const winner = finished[0];
		const moves = winner.finishedMoves;
		els.winnerText.textContent = `${winner.name || `Player ${winner.posn}`} wins in ${moves} move${moves === 1 ? "" : "s"}`;
	} else {
		els.winnerText.textContent = "Game over";
	}
	els.winnerBanner.hidden = false;
}

function clearChat() {
	els.chatLog.replaceChildren(el("p", { class: "chat-empty", text: "No messages yet." }));
}

function appendChat(text) {
	const message = String(text || "");
	if (!message.trim()) return;

	const placeholder = els.chatLog.querySelector(".chat-empty");
	if (placeholder) placeholder.remove();

	const nearBottom = els.chatLog.scrollTop + els.chatLog.clientHeight
		>= els.chatLog.scrollHeight - 24;

	const line = el("div", { class: "chat-line" });
	const match = /^<<([^>]*)>>\s?([\s\S]*)$/.exec(message);

	if (match) {
		const author = match[1];
		const body = match[2];
		const player = Array.from(state.players.values())
			.find((candidate) => candidate.name === author);
		const dot = pegDot(player ? player.color : 0);
		if (!player) {
			dot.classList.remove("empty");
			dot.style.background = "#8a9096";
		}
		line.append(dot);

		const copy = el("span", { class: "chat-copy" });
		copy.append(el("span", {
			class: `chat-author${author === state.myName ? " mine" : ""}`,
			text: `${author}: `,
		}));
		copy.append(document.createTextNode(body));
		line.append(copy);
	} else {
		line.classList.add("system");
		line.textContent = message;
	}

	els.chatLog.append(line);
	while (els.chatLog.children.length > 300) {
		els.chatLog.firstElementChild.remove();
	}
	if (nearBottom) els.chatLog.scrollTop = els.chatLog.scrollHeight;
}

function toast(message) {
	clearTimeout(toastTimer);
	clearTimeout(toastHideTimer);
	els.toast.textContent = message;
	els.toast.hidden = false;
	requestAnimationFrame(() => els.toast.classList.add("show"));
	toastTimer = setTimeout(() => {
		els.toast.classList.remove("show");
		toastHideTimer = setTimeout(() => {
			els.toast.hidden = true;
		}, 260);
	}, 2200);
}

function openModal({ title = "", body = null, actions = [], dismissible = true } = {}) {
	modalDismissible = dismissible;
	els.modalTitle.textContent = title;

	els.modalBody.replaceChildren();
	els.modalBody.scrollTop = 0;
	if (typeof body === "string") els.modalBody.innerHTML = body;
	else if (body) els.modalBody.append(body);

	els.modalActions.replaceChildren(...actions);
	els.modalActions.hidden = actions.length === 0;
	els.modalClose.hidden = !dismissible;

	if (!els.modal.open) els.modal.showModal();

	requestAnimationFrame(() => {
		const focus = els.modalBody.querySelector("input, select, textarea, button");
		if (focus) {
			focus.focus();
			if (typeof focus.select === "function") focus.select();
		}
	});
}

function closeModal() {
	if (els.modal.open) els.modal.close();
	modalDismissible = true;
}

function openNameDialog({ forced = false, suggested = "" } = {}) {
	const input = el("input", {
		id: "modalNameInput",
		type: "text",
		maxlength: 40,
		value: suggested || state.myName || prefs.name || "",
		autocomplete: "nickname",
		placeholder: "Player",
		required: true,
	});
	const error = el("p", { class: "form-error", hidden: true });
	const form = el("form", { id: "modalNameForm" });
	form.append(
		el("label", { class: "modal-field", for: "modalNameInput" },
			el("span", { text: "Your name" }),
			input),
		error,
	);

	const save = modalButton(forced ? "Use This Name" : "Save", {
		primary: true,
		submit: true,
		form: "modalNameForm",
	});

	form.addEventListener("submit", (event) => {
		event.preventDefault();
		const name = cleanName(input.value);
		if (!name) {
			error.textContent = "Please enter a name.";
			error.hidden = false;
			input.focus();
			return;
		}
		state.myName = name;
		const color = state.myColor || prefs.color || 1;
		savePrefs(name, color);
		conn.send(`change_name ${name}`);
		closeModal();
		toast("Name updated.");
	});
	input.addEventListener("input", () => {
		error.hidden = true;
	});

	const body = el("div");
	if (forced) {
		body.append(el("p", {
			text: "That name is already in use. Please choose another one.",
		}));
	}
	body.append(form);

	const actions = [];
	if (!forced) actions.push(modalButton("Cancel", { onClick: closeModal }));
	actions.push(save);

	openModal({
		title: forced ? "Choose a Name" : "Change Name",
		body,
		actions,
		dismissible: !forced,
	});
}

function openColorDialog({ forced = false } = {}) {
	let picked = forced ? 0 : (state.myColor || prefs.color || 0);
	const swatches = el("div", {
		class: "color-swatches",
		role: "radiogroup",
		"aria-label": "Peg color",
	});
	const save = modalButton(forced ? "Use This Color" : "Save", {
		primary: true,
		disabled: !picked,
	});

	colorSwatches(swatches, picked, (color) => {
		picked = color;
		save.disabled = false;
	});

	save.addEventListener("click", () => {
		if (!picked) return;
		state.myColor = picked;
		savePrefs(state.myName, picked);
		conn.send(`change_color ${picked}`);
		closeModal();
		toast("Color updated.");
	});

	const body = el("div");
	if (forced) {
		body.append(el("p", {
			text: "That color is already taken. Please choose another one.",
		}));
	}
	body.append(el("div", { class: "modal-field" },
		el("span", { text: "Peg color" }),
		swatches));

	const actions = [];
	if (!forced) actions.push(modalButton("Cancel", { onClick: closeModal }));
	actions.push(save);


	openModal({
		title: forced ? "Choose a Color" : "Change Color",
		body,
		actions,
		dismissible: !forced,
	});
}

function confirmDialog({ title, message, confirmLabel = "OK", danger = false, onConfirm }) {
	const body = el("p", { text: message });
	const actions = [
		modalButton("Cancel", { onClick: closeModal }),
		modalButton(confirmLabel, {
			primary: !danger,
			danger,
			onClick: () => {
				closeModal();
				onConfirm?.();
			},
		}),
	];
	openModal({ title, body, actions });
}

const HOW_TO_PLAY = `
	<div class="help">
		<h3>Goal</h3>
		<p>Move all ten of your pegs across the board into the opposite triangle. The first player to get every peg home wins.</p>

		<h3>Moving</h3>
		<p>A peg can move to any adjacent empty hole. It can also jump over a single peg into the empty hole directly beyond it. Jumps can be chained together in one move, changing direction as you go.</p>
		<p>Click one of your pegs, then click each hole along the path. Double-click the final hole — or press Enter or the Move button — to make the move. Press Escape or Cancel to clear the selection.</p>

		<h3>Goal Triangles</h3>
		<p>You may move through your own goal at any time. Depending on the host's settings, you may also pass through or stop in an opponent's goal.</p>

		<h3>Variations</h3>
		<p><strong>Long jumps</strong> allow a symmetrical jump with any number of empty holes on either side of the jumped peg. The host chooses whether long jumps, passing through opponents' goals, and stopping in opponents' goals are allowed.</p>

		<h3>Controls</h3>
		<p>Use <strong>Game</strong> to undo, redo, or leave the board. Use <strong>Player</strong> to change your name or peg color. Chat with everyone at the bottom of the sidebar.</p>
	</div>
`;

const ABOUT_HTML = `
	<div class="help">
		<p><strong>Cheech</strong> is a networked Chinese Checkers game with a shared C++ engine and native apps for desktop and iOS.</p>
		<p>Cheech is open source under the MIT license. Source and issues: <a href="https://github.com/benjie-git/cheech" target="_blank" rel="noopener">github.com/benjie-git/cheech</a>.</p>
		<p class="muted">Built for playing with any combination of bots and friends, near or far.</p>
	</div>
`;

function openHowToPlay() {
	openModal({
		title: "How to Play",
		body: HOW_TO_PLAY,
		actions: [modalButton("Close", { primary: true, onClick: closeModal })],
	});
}

function openAbout() {
	openModal({
		title: "About Cheech",
		body: ABOUT_HTML,
		actions: [modalButton("Close", { primary: true, onClick: closeModal })],
	});
}

function showLostDialog() {
	const body = el("div", { class: "help" },
		el("p", { text: "The connection to the board was lost. Your seat may have been released while you were away." }),
		el("p", { text: "You can try to rejoin the same game, or go back to the join screen." }));

	const actions = [
		modalButton("Back to Join", {
			onClick: () => {
				closeModal();
				conn.disconnect();
				state.joined = false;
				showJoinView({ error: "The connection to the board was lost." });
			},
		}),
		modalButton("Rejoin", {
			primary: true,
			onClick: () => {
				closeModal();
				beginSession();
			},
		}),
	];

	openModal({
		title: "Connection Lost",
		body,
		actions,
		dismissible: false,
	});
}

function pathsEqual(a, b) {
	return a.length === b.length && a.every((hole, index) => hole === b[index]);
}

function clearSelection() {
	state.selection = [];
	state.telegraph = [];
	boardView.setSelection([]);
	boardView.setTelegraph([]);
	updateMoveControls();
}

function commitSelection() {
	if (state.selection.length < 2) return;
	conn.send("commit");
	clearSelection();
}

function cancelSelection() {
	conn.send("clear");
	clearSelection();
}

function updateMoveControls() {
	const playing = canPlay();
	const selected = state.selection.length;

	els.moveControls.hidden = !(playing && selected > 0);
	els.commitMove.disabled = selected < 2;

	let hint = "";
	if (playing) {
		hint = selected > 0
			? `${selected} hole${selected === 1 ? "" : "s"} selected — click another hole or press Enter.`
			: "Select one of your pegs. Double-click a destination to move.";
	}
	els.boardHint.textContent = hint;
	els.boardHint.hidden = !hint;
}

function boardInteraction(event, isDouble) {
	if (event.button !== 0) return;
	if (!state.joined || !state.hasBoard || boardView.isRotating()) return;

	const rect = els.board.getBoundingClientRect();
	const hole = boardView.holeAt(event.clientX - rect.left, event.clientY - rect.top);
	if (hole < 0) return;

	if (state.spectator) {
		toast("You’re spectating this game.");
		return;
	}
	if (state.status === STATUS.WAITING) {
		toast("Waiting for players.");
		return;
	}
	if (allPlayersFinished()) {
		toast("The game is over.");
		return;
	}
	if (!state.myPosn || state.currentPlayer !== state.myPosn) {
		toast(state.currentPlayer > 0
			? `It’s ${playerName(state.currentPlayer)}’s turn.`
			: "It’s not your turn.");
		return;
	}

	if (isDouble) {
		clearSelection();
		conn.send(`dblclick ${hole}`);
	} else {
		conn.send(`click ${hole}`);
	}
}

function onBoardPointerDown(event) {
	boardInteraction(event, false);
}

function onBoardDoubleClick(event) {
	boardInteraction(event, true);
}

function handleAction(action) {
	switch (action) {
		case "undo":
			if (!canUseHistory()) return;
			clearSelection();
			conn.send("undo");
			break;

		case "redo":
			if (!canUseHistory()) return;
			clearSelection();
			conn.send("redo");
			break;

		case "leave":
			if (!state.joined) return;
			confirmDialog({
				title: "Leave the Board?",
				message: "Your seat will be released. You can rejoin later if the game still has room.",
				confirmLabel: "Leave",
				danger: true,
				onConfirm: () => {
					state.joined = false;
					conn.disconnect({ notifyServer: true });
					showJoinView({ message: "You left the board." });
				},
			});
			break;

		case "name":
			if (canChangeIdentity()) openNameDialog();
			break;

		case "color":
			if (canChangeIdentity()) openColorDialog();
			break;

		case "howto":
			openHowToPlay();
			break;

		case "about":
			openAbout();
			break;
	}
}

function closeMenus() {
	for (const menu of menus) menu.open = false;
}

function initMenus() {
	for (const menu of menus) {
		menu.addEventListener("toggle", () => {
			if (!menu.open) return;
			for (const other of menus) {
				if (other !== menu) other.open = false;
			}
		});
	}
}

function onDocumentClick(event) {
	const action = event.target.closest("[data-action]");
	if (action) {
		event.preventDefault();
		closeMenus();
		handleAction(action.dataset.action);
		return;
	}
	if (!event.target.closest(".menu")) closeMenus();
}

function onKeyDown(event) {
	if (els.modal.open) {
		if (event.key === "Escape" && modalDismissible) {
			event.preventDefault();
			closeModal();
		}
		return;
	}

	const target = event.target;
	if (target instanceof Element && target.closest("input, textarea, select, [contenteditable='true']")) {
		return;
	}

	if (event.key === "Escape") {
		if (state.selection.length > 0) {
			event.preventDefault();
			cancelSelection();
		} else {
			closeMenus();
		}
		return;
	}

	if (event.key === "Enter" && state.selection.length >= 2) {
		event.preventDefault();
		commitSelection();
		return;
	}

	if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === "z") {
		event.preventDefault();
		if (!canUseHistory()) return;
		clearSelection();
		conn.send(event.shiftKey ? "redo" : "undo");
	}
}

function onJoinSubmit(event) {
	event.preventDefault();

	const name = cleanName(els.joinName.value);
	if (!name) {
		showJoinError("Please enter a name.");
		els.joinName.focus();
		return;
	}

	state.myName = name;
	state.myColor = joinColor;
	state.spectator = els.joinSpectator.checked;
	savePrefs(name, joinColor);
	hideJoinError();
	beginSession();
}

function onChatSubmit(event) {
	event.preventDefault();
	const text = els.chatInput.value.trim();
	if (!text) return;
	if (conn.send(`chat ${text}`)) els.chatInput.value = "";
}

let suspended = false;

function onPageHide() {
	if (!conn.running) return;
	suspended = true;
	conn.disconnect({ notifyServer: true });
}

function onPageShow(event) {
	if (!event.persisted || !suspended) return;
	suspended = false;
	showJoinView({ message: "The connection was closed while the page was away." });
}

function init() {
	els.joinName.value = prefs.name;
	joinColor = prefs.color || 1;
	renderJoinColors();
	resetSessionState();
	updateChrome();

	els.joinForm.addEventListener("submit", onJoinSubmit);
	els.chatForm.addEventListener("submit", onChatSubmit);
	els.board.addEventListener("pointerdown", onBoardPointerDown);
	els.board.addEventListener("dblclick", onBoardDoubleClick);
	els.cancelMove.addEventListener("click", cancelSelection);
	els.commitMove.addEventListener("click", commitSelection);

	els.modalClose.addEventListener("click", closeModal);
	els.modal.addEventListener("click", (event) => {
		if (event.target === els.modal && modalDismissible) closeModal();
	});
	els.modal.addEventListener("cancel", (event) => {
		if (!modalDismissible) event.preventDefault();
		else closeModal();
	});

	document.addEventListener("click", onDocumentClick);
	document.addEventListener("keydown", onKeyDown);
	window.addEventListener("pagehide", onPageHide);
	window.addEventListener("pageshow", onPageShow);

	initMenus();
}

init();
