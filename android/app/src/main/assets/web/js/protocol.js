const CONTROL_CHARS = /[\u0000-\u001f\u007f]/g;
const MAX_NAME = 40;
const COMMAND_GRACE_MS = 350;
const POLL_WATCHDOG_MS = 25000;

export function cleanName(value) {
	return String(value ?? "")
		.replace(CONTROL_CHARS, " ")
		.replace(/\s+/g, " ")
		.trim()
		.slice(0, MAX_NAME);
}

export class CheechConnection {
	constructor({ endpoint = "api" } = {}) {
		this.endpoint = endpoint;
		this.onCommand = () => {};
		this.onStatus = () => {};
		this.clientId = 0;
		this.running = false;
		this.queue = [];
		this.busy = false;
		this.polling = false;
		this.failures = 0;
		this.generation = 0;
		this.commandToken = 0;
		this.status = "idle";
		this._pollController = null;
		this._retryTimer = 0;
	}

	connect({ name, color, spectator }) {
		this._reset();
		this.running = true;
		this._setStatus("connecting");
		this.send(`init ${color} ${spectator ? 1 : 0} ${cleanName(name) || "Player"}`);
	}

	disconnect({ notifyServer = false } = {}) {
		if (notifyServer && this.clientId) {
			try {
				fetch(this._url(`${this.clientId} leave`), {
					cache: "no-store",
					keepalive: true,
				}).catch(() => {});
			} catch (_) {
				// Leave is best-effort; the server also times out idle sessions.
			}
		}
		this._reset();
		this._setStatus("idle");
	}

	send(command) {
		if (!this.running) return false;
		const text = String(command ?? "").replace(/[\r\n]+/g, " ").trim();
		if (!text) return false;
		this.queue.push(text);
		this._pump();
		return true;
	}

	_url(payload) {
		return `${this.endpoint}?q=${encodeURIComponent(payload)}`;
	}

	async _get(payload, options = {}) {
		const response = await fetch(this._url(payload), {
			method: "GET",
			cache: "no-store",
			...options,
		});
		if (!response.ok) throw new Error(`HTTP ${response.status}`);
		return response.text();
	}

	_setStatus(status) {
		if (status === this.status) return;
		this.status = status;
		this.onStatus(status);
	}

	_reset() {
		this.running = false;
		this.generation++;
		this.clientId = 0;
		this.queue.length = 0;
		this.busy = false;
		this.polling = false;
		this.failures = 0;
		this.commandToken++;
		clearTimeout(this._retryTimer);
		this._retryTimer = 0;
		if (this._pollController) {
			this._pollController.abort();
			this._pollController = null;
		}
	}

	_pump() {
		if (!this.running) return;
		if (this.busy) return;
		if (this.queue.length) {
			this._sendCommand();
			return;
		}
		if (!this.polling && this.clientId) this._sendPoll();
	}

	_sendCommand() {
		const generation = this.generation;
		const command = this.queue.shift();
		const token = ++this.commandToken;
		let settled = false;
		this.busy = true;

		const finish = (text) => {
			if (settled) return;
			settled = true;
			clearTimeout(grace);
			if (generation !== this.generation) return;
			if (token === this.commandToken) this.busy = false;
			this.failures = 0;
			if (text) this._receive(text);
			this._pump();
		};

		const grace = setTimeout(() => {
			if (settled || generation !== this.generation) return;
			if (token === this.commandToken) this.busy = false;
			this._pump();
		}, COMMAND_GRACE_MS);

		this._get(`${this.clientId} ${command}`)
			.then((text) => finish(text))
			.catch(() => {
				if (settled || generation !== this.generation) return;
				settled = true;
				clearTimeout(grace);
				if (token === this.commandToken) this.busy = false;
				this._fail();
			});
	}

	_sendPoll() {
		const generation = this.generation;
		const controller = new AbortController();
		let settled = false;
		this.polling = true;
		this._pollController = controller;

		const finish = (text) => {
			if (settled) return;
			settled = true;
			clearTimeout(watchdog);
			if (this._pollController === controller) this._pollController = null;
			if (generation !== this.generation) return;
			this.polling = false;
			this.failures = 0;
			this._setStatus("online");
			if (text) this._receive(text);
			this._pump();
		};

		const watchdog = setTimeout(() => {
			if (settled || generation !== this.generation) return;
			controller.abort();
		}, POLL_WATCHDOG_MS);

		this._get(`${this.clientId} none`, { signal: controller.signal })
			.then((text) => finish(text))
			.catch(() => {
				if (settled || generation !== this.generation) return;
				settled = true;
				clearTimeout(watchdog);
				if (this._pollController === controller) this._pollController = null;
				this.polling = false;
				this._fail();
			});
	}

	_fail() {
		if (!this.running) return;
		this.failures++;
		if (this.failures >= 5) {
			this.running = false;
			this._setStatus("lost");
			return;
		}
		this._setStatus(this.failures >= 2 ? "reconnecting" : "offline");
		clearTimeout(this._retryTimer);
		const delay = Math.min(8000, 500 * Math.pow(2, this.failures - 1));
		this._retryTimer = setTimeout(() => this._pump(), delay);
	}

	_receive(text) {
		const raw = String(text).replace(/\r/g, "").replace(/\n+$/, "");
		if (!raw) return;

		const split = raw.indexOf(" ");
		const name = split === -1 ? raw : raw.slice(0, split);
		const args = split === -1 ? "" : raw.slice(split + 1);

		if (name === "id") {
			const id = parseInt(args, 10);
			if (Number.isFinite(id) && id > 0) {
				this.clientId = id;
				this._setStatus("online");
			}
		}

		this.onCommand(name, args, raw);

		if (name === "exit") {
			this._reset();
			this._setStatus("exited");
		}
	}
}
