import Foundation
import SwiftUI

enum Screen {
	case setup
	case game
}

// Which half of the merged setup page is showing.
enum SetupMode: Int {
	case start
	case join
}

// Best-effort local Wi-Fi address for the "Invite a Friend" page.
func localIPAddress() -> String? {
	var address: String?
	var ifaddr: UnsafeMutablePointer<ifaddrs>?
	guard getifaddrs(&ifaddr) == 0, let first = ifaddr else { return nil }
	defer { freeifaddrs(ifaddr) }
	var pointer: UnsafeMutablePointer<ifaddrs>? = first
	while let current = pointer {
		let interface = current.pointee
		if let addr = interface.ifa_addr, addr.pointee.sa_family == UInt8(AF_INET) {
			if String(cString: interface.ifa_name) == "en0" {
				var buffer = [CChar](repeating: 0, count: Int(NI_MAXHOST))
				if getnameinfo(
					addr, socklen_t(addr.pointee.sa_len),
					&buffer, socklen_t(buffer.count),
					nil, 0, NI_NUMERICHOST
				) == 0 {
					address = String(cString: buffer)
					break
				}
			}
		}
		pointer = interface.ifa_next
	}
	return address
}

// What controls a seat in a locally hosted game.
enum SeatKind: Int, CaseIterable, Identifiable, Codable {
	case human
	case computer
	case remote

	var id: Int { rawValue }
}

// One seat in a locally hosted game.
struct SeatConfig: Identifiable, Equatable, Codable {
	var id = UUID()
	var kind: SeatKind
	var botType: String
	var name: String
	var color: Int
	// The last name used while this seat was Human, so switching a seat back to
	// Human restores the player's own name instead of the computer default.
	var humanName: String?
	// Computer-seat skill level (1...4, 1 = Best).  nil means Best.  Optional so
	// previously saved seat setups still decode.
	var skill: Int?
}

// A computer seat's skill is a level 1...4 (Best/Great/Mid/Nerfed).  How a
// level weakens a bot depends on the family: Mean and Friendly keep their
// four-move search and widen the tier count (1,2,4,8), while LookAhead drops
// from a four-move to a one-move search for the weaker two levels, because a
// shallow LookAhead is the only thing that reads as plausibly weak rather than
// random.  The core still speaks in smarts percentages, so this maps a level
// to an effective (type, smarts) pair just before the seat is created.
enum BotSkill {
	static func level(_ raw: Int?) -> Int {
		min(max(raw ?? 1, 1), 4)
	}

	static func label(for level: Int) -> String {
		["Best", "Great", "Mid", "Nerfed"][Self.level(level) - 1]
	}

	static func isLookAhead(_ type: String) -> Bool {
		type.hasPrefix("LookAhead")
	}

	// Number of distinct score tiers the level allows, for display.
	static func tiers(type: String, level: Int) -> Int {
		let l = Self.level(level)
		return isLookAhead(type) ? [1, 2, 1, 4][l - 1] : (1 << (l - 1))
	}

	// Effective bot type and smarts percentage for a level.
	static func resolved(type: String, level: Int) -> (type: String, smarts: Int) {
		let l = Self.level(level)
		if isLookAhead(type) {
			return (l <= 2 ? "LookAhead(4)" : "LookAhead(2)", [100, 90, 100, 80][l - 1])
		}
		return (type, [100, 90, 80, 70][l - 1])
	}
}

// A destructive action that should be confirmed once a game is underway.
enum GameAction: Identifiable {
	case leave
	case restart
	case rotate
	case shuffle

	var id: Int { rawValue }

	private var rawValue: Int {
		switch self {
		case .leave: return 0
		case .restart: return 1
		case .rotate: return 2
		case .shuffle: return 3
		}
	}
}

private enum PrefKey {
	static let playerName = "playerName"
	static let playerColor = "playerColor"
	static let hostPort = "hostPort"
	static let numPlayers = "numPlayers"
	static let longJumps = "longJumps"
	static let hopOthers = "hopOthers"
	static let stopOthers = "stopOthers"
	static let joinHost = "joinHost"
	static let joinPort = "joinPort"
	static let seats = "seats"
	static let lastBotType = "lastBotType"
	static let opponentStepMs = "opponentStepMs"
	// Opaque snapshot of an all-local game, written when the app backgrounds and
	// resumed on the next launch (see persistLocalGame()/resumeSavedGameIfNeeded()).
	static let savedGame = "savedGame"
}

final class SessionModel: NSObject, ObservableObject, CheechSessionDelegate {
	let session = CheechSession()
	let animator = MoveAnimator()
	private var lastMoveSerial = 0
	private var lastTapHole = -1
	private var lastTapTime = Date.distantPast
	private var lastTapWasTerminal = false

	// The player whose move is currently replaying on screen.  Kept until the
	// animation finishes so the turn overlay lags behind the server, which
	// advances the turn (and lets bots start thinking) as soon as a move lands.
	@Published var animatingPlayer = 0

	@Published var screen: Screen = .setup
	@Published var setupMode: SetupMode = .start
	@Published var showInvite = false
	@Published var updateToken = 0
	@Published var messages: [String] = []
	@Published var showProfile = false
	@Published var spectator = false
	@Published var fullScreen = false
	// Whether the server log strip is revealed on the game screen.  Hidden for
	// fully local games and behind an (i) button otherwise.
	@Published var showServerLog = false
	// In-game "Setup Game" sheet (change rules/player count, add bots).
	@Published var showGameSetup = false
	// A destructive action awaiting confirmation (only when a game is
	// underway).  The game screen presents a dialog while this is non-nil.
	@Published var confirmAction: GameAction? = nil
	// Backs the dialog's presentation so its binding setter never mutates
	// another @Published property during a SwiftUI view update.
	@Published var showConfirm = false

	// Preferences (persisted to UserDefaults on every change).
	@Published var playerName: String { didSet { save(playerName, PrefKey.playerName) } }
	@Published var playerColor: Int { didSet { save(playerColor, PrefKey.playerColor) } }
	@Published var hostPort: Int { didSet { save(hostPort, PrefKey.hostPort) } }
	@Published var numPlayers: Int {
		didSet {
			save(numPlayers, PrefKey.numPlayers)
			if numPlayers != oldValue { syncSeatCount() }
		}
	}
	@Published var longJumps: Bool { didSet { save(longJumps, PrefKey.longJumps) } }
	@Published var hopOthers: Bool { didSet { save(hopOthers, PrefKey.hopOthers) } }
	@Published var stopOthers: Bool { didSet { save(stopOthers, PrefKey.stopOthers) } }
	@Published var joinHost: String { didSet { save(joinHost, PrefKey.joinHost) } }
	@Published var joinPort: Int { didSet { save(joinPort, PrefKey.joinPort) } }

	// Seat setup for a locally hosted game (Human / Computer / Remote).
	@Published var seats: [SeatConfig] = [] {
		didSet { saveSeats() }
	}

	// The bot type most recently chosen, used as the default for new
	// computer seats.
	@Published var lastBotType: String {
		didSet { save(lastBotType, PrefKey.lastBotType) }
	}

	// Opponent move animation speed, in milliseconds per hop (0...500); the
	// destination rest time is twice this.
	@Published var opponentStepMs: Int {
		didSet { save(opponentStepMs, PrefKey.opponentStepMs) }
	}

	override init() {
		let defaults = UserDefaults.standard
		playerName = defaults.string(forKey: PrefKey.playerName) ?? "Player"
		playerColor = defaults.object(forKey: PrefKey.playerColor) as? Int ?? 1
		hostPort = defaults.object(forKey: PrefKey.hostPort) as? Int ?? 3838
		longJumps = defaults.object(forKey: PrefKey.longJumps) as? Bool ?? false
		hopOthers = defaults.object(forKey: PrefKey.hopOthers) as? Bool ?? true
		stopOthers = defaults.object(forKey: PrefKey.stopOthers) as? Bool ?? true
		joinHost = defaults.string(forKey: PrefKey.joinHost) ?? "127.0.0.1"
		joinPort = defaults.object(forKey: PrefKey.joinPort) as? Int ?? 3838
		lastBotType = defaults.string(forKey: PrefKey.lastBotType) ?? "LookAhead(3)"
		opponentStepMs = min(max(defaults.object(forKey: PrefKey.opponentStepMs) as? Int ?? 250, 0), 500)
		if let data = defaults.data(forKey: PrefKey.seats),
		   let decoded = try? JSONDecoder().decode([SeatConfig].self, from: data),
		   decoded.count >= 2, decoded.count <= 6 {
			seats = decoded
			numPlayers = decoded.count
		} else {
			seats = []
			numPlayers = defaults.object(forKey: PrefKey.numPlayers) as? Int ?? 3
		}
		super.init()
		session.delegate = self
		syncSeatCount()
		resumeSavedGameIfNeeded()
	}

	private func save(_ value: Any, _ key: String) {
		UserDefaults.standard.set(value, forKey: key)
	}

	private func saveSeats() {
		if let data = try? JSONEncoder().encode(seats) {
			UserDefaults.standard.set(data, forKey: PrefKey.seats)
		}
	}

	private func syncSeatCount() {
		let count = numPlayers
		if seats.count < count {
			for i in seats.count..<count {
				let defaultName = "Player \(i + 1)"
				seats.append(SeatConfig(
					kind: .human,
					botType: lastBotType,
					name: defaultName,
					color: i % 8 + 1,
					humanName: defaultName,
					skill: nil
				))
			}
		} else if seats.count > count {
			seats.removeLast(seats.count - count)
		}
	}

	func cheechSessionDidUpdate(_ session: CheechSession) {
		updateToken += 1

		// A finished all-local game should not be auto-resumed on next launch.
		if session.status == .won {
			clearSavedGame()
		}

		let serial = session.moveSerial
		if serial != lastMoveSerial {
			lastMoveSerial = serial
			let path = session.lastMove().map { $0.intValue }
			if path.count > 1 {
				// Only replay moves made by other players.  The local
				// player's own move snaps into place so the computer's
				// immediate reply cannot interrupt it mid-flight.
				let mover = session.player(atHole: path[path.count - 1])
				if mover != session.myPlayerNumber {
					animatingPlayer = Int(mover)
					// Once control passes to a local human, drop the resting
					// delay so they can start tapping as soon as the peg lands.
					animator.start(
						path: path,
						step: Double(session.animationStepMs) / 1000.0,
						done: isLocalHumanTurn ? 0 : Double(session.animationDoneMs) / 1000.0
					) { [weak self] in
						self?.animatingPlayer = 0
					}
				}
			}
		}
	}

	func cheechSession(_ session: CheechSession, didReceiveMessage message: String) {
		messages.append(message)
	}

	func joinGame(spectator: Bool) {
		lastMoveSerial = session.moveSerial
		animator.stop()
		animatingPlayer = 0
		showServerLog = false
		session.setAnimationStepMs(opponentStepMs)
		session.joinHost(
			joinHost,
			port: UInt16(joinPort),
			spectator: spectator,
			playerName: playerName,
			color: playerColor
		)
		screen = .game
	}

	func leave() {
		clearSavedGame()
		session.leave()
		animator.stop()
		animatingPlayer = 0
		lastTapHole = -1
		lastTapWasTerminal = false
		messages.removeAll()
		fullScreen = false
		confirmAction = nil
		showConfirm = false
		showServerLog = false
		screen = .setup
	}

	// Handle an invite link such as cheech://join?host=192.168.1.5&port=3838
	// (also accepts cheech://192.168.1.5:3838) by joining that game.
	func handle(url: URL) {
		guard url.scheme?.lowercased() == "cheech" else { return }
		let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
		let items = components?.queryItems ?? []
		var host = items.first(where: { $0.name == "host" })?.value
		var port = items.first(where: { $0.name == "port" })?.value
		if host == nil, let urlHost = url.host, urlHost.lowercased() != "join" {
			host = urlHost
		}
		if port == nil { port = components?.port.map(String.init) }
		guard let host, !host.isEmpty, let portString = port, let portNumber = Int(portString),
		      portNumber > 0, portNumber <= 65535 else { return }
		joinFromInvite(host: host, port: portNumber)
	}

	private func joinFromInvite(host: String, port: Int) {
		if screen == .game { session.leave() }
		animator.stop()
		animatingPlayer = 0
		joinHost = host
		joinPort = port
		setupMode = .join
		joinGame(spectator: false)
	}

	func startGame() {
		clearSavedGame()
		lastMoveSerial = session.moveSerial
		animator.stop()
		animatingPlayer = 0
		lastTapHole = -1
		lastTapWasTerminal = false
		messages.removeAll()
		showServerLog = false
		let seatSpecs: [CheechSeat] = seats.map { seat in
			switch seat.kind {
			case .human:
				return CheechSeat.human(withName: seat.name, color: seat.color)
			case .computer:
				let resolved = BotSkill.resolved(type: seat.botType, level: seat.skill ?? 1)
				let spec = CheechSeat.computer(
					withType: resolved.type,
					name: CheechSession.defaultName(forComputerType: resolved.type),
					color: seat.color
				)
				spec.smarts = resolved.smarts
				return spec
			case .remote:
				return CheechSeat.remote()
			}
		}
		session.setAnimationStepMs(opponentStepMs)
		session.startGame(
			onPort: UInt16(hostPort),
			numPlayers: numPlayers,
			longJumps: longJumps,
			hopOthers: hopOthers,
			stopOthers: hopOthers && stopOthers,
			seats: seatSpecs
		)
		screen = .game
	}

	// MARK: - Local game persistence

	// Writes (or removes) the snapshot of an in-progress all-local game.  Called
	// when the app leaves the foreground; a networked game yields no snapshot, so
	// any previous save is dropped.
	func persistLocalGame() {
		if let save = session.localGameSave() {
			UserDefaults.standard.set(save, forKey: PrefKey.savedGame)
		} else {
			clearSavedGame()
		}
	}

	private func clearSavedGame() {
		UserDefaults.standard.removeObject(forKey: PrefKey.savedGame)
	}

	// Resumes an all-local game saved by a previous run, if any.  The snapshot
	// carries the seats, rules, board and turn, so no setup is needed.
	private func resumeSavedGameIfNeeded() {
		guard let save = UserDefaults.standard.string(forKey: PrefKey.savedGame),
		      !save.isEmpty else { return }
		lastMoveSerial = session.moveSerial
		animator.stop()
		animatingPlayer = 0
		lastTapHole = -1
		lastTapWasTerminal = false
		messages.removeAll()
		showServerLog = false
		session.setAnimationStepMs(opponentStepMs)
		guard session.resumeLocalGame(save) else {
			clearSavedGame()
			return
		}
		screen = .game
	}

	func updateProfile(name: String, color: Int) {
		playerName = name
		playerColor = color
		session.changeName(name)
		session.changeColor(color)
	}

	// In-game setup (see CheechSession's "In-game setup" methods).  Changing
	// the player count or rules applies to the running server and restarts the
	// board; computer players connect to the same host as this client.
	func reconfigureGame(numPlayers: Int, longJumps: Bool, hopOthers: Bool, stopOthers: Bool) {
		session.reconfigureGame(
			numPlayers: numPlayers,
			longJumps: longJumps,
			hopOthers: hopOthers,
			stopOthers: hopOthers && stopOthers
		)
	}

	func addComputerPlayer(type: String, name: String, color: Int) {
		session.addComputerPlayer(ofType: type, name: name, color: color)
	}

	func removeComputerPlayers() {
		session.removeComputerPlayers()
	}

	// Whether the local device may currently make a move: a human seat that it
	// controls.  Checked while the previous move is replaying so the player can
	// begin tapping immediately rather than waiting for the animation.
	var isLocalHumanTurn: Bool {
		guard session.connected else { return false }
		if session.isHost {
			return session.activeSeatKind == .human
		}
		return !session.isSpectator
			&& session.myPlayerNumber != 0
			&& session.currentPlayer == session.myPlayerNumber
	}

	// Whether the server log can be shown at all.  A fully local game (hosted
	// with no remote seats) has no server log.
	var canShowServerLog: Bool {
		!session.isHost || session.hasRemoteSeat
	}

	// Removes just the last hop from the in-progress move path (backspace/delete).
	func removeLastHop() {
		lastTapHole = -1
		lastTapWasTerminal = false
		session.removeLastHop()
	}

	func handleTap(hole: Int) {
		let now = Date()
		let isDoubleTap = hole == lastTapHole && now.timeIntervalSince(lastTapTime) < 0.4
		lastTapHole = hole
		lastTapTime = now

		if isDoubleTap {
			let selected = session.selectedHoles().map { $0.intValue }
			if selected.count >= 2 && selected.last == hole {
				lastTapHole = -1
				lastTapWasTerminal = false
				session.confirmMove()
				return
			}
			if lastTapWasTerminal {
				// The first tap deselected a hole that was already the end of
				// the move path.  Put it back, then confirm the full path.
				lastTapHole = -1
				lastTapWasTerminal = false
				session.tapHole(hole)
				session.confirmMove()
				return
			}
		}

		let selected = session.selectedHoles().map { $0.intValue }
		lastTapWasTerminal = selected.count >= 2 && selected.last == hole
		session.tapHole(hole)
	}
}
