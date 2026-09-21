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
	// A destructive action awaiting confirmation (only when a game is
	// underway).  The game screen presents a dialog while this is non-nil.
	@Published var confirmAction: GameAction? = nil

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
		hostPort = defaults.object(forKey: PrefKey.hostPort) as? Int ?? 34567
		longJumps = defaults.object(forKey: PrefKey.longJumps) as? Bool ?? false
		hopOthers = defaults.object(forKey: PrefKey.hopOthers) as? Bool ?? true
		stopOthers = defaults.object(forKey: PrefKey.stopOthers) as? Bool ?? true
		joinHost = defaults.string(forKey: PrefKey.joinHost) ?? "127.0.0.1"
		joinPort = defaults.object(forKey: PrefKey.joinPort) as? Int ?? 34567
		lastBotType = defaults.string(forKey: PrefKey.lastBotType)
			?? GameScreenView.botTypes.first ?? "LookAhead(2)"
		opponentStepMs = min(max(defaults.object(forKey: PrefKey.opponentStepMs) as? Int ?? 250, 0), 500)
		if let data = defaults.data(forKey: PrefKey.seats),
		   let decoded = try? JSONDecoder().decode([SeatConfig].self, from: data),
		   decoded.count >= 2, decoded.count <= 6 {
			seats = decoded
			numPlayers = decoded.count
		} else {
			seats = []
			numPlayers = defaults.object(forKey: PrefKey.numPlayers) as? Int ?? 4
		}
		super.init()
		session.delegate = self
		syncSeatCount()
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
				seats.append(SeatConfig(
					kind: .human,
					botType: lastBotType,
					name: "Player \(i + 1)",
					color: i % 8 + 1
				))
			}
		} else if seats.count > count {
			seats.removeLast(seats.count - count)
		}
	}

	func cheechSessionDidUpdate(_ session: CheechSession) {
		updateToken += 1

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
					animator.start(
						path: path,
						step: Double(session.animationStepMs) / 1000.0,
						done: Double(session.animationDoneMs) / 1000.0
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
		session.leave()
		animator.stop()
		animatingPlayer = 0
		lastTapHole = -1
		lastTapWasTerminal = false
		messages.removeAll()
		fullScreen = false
		confirmAction = nil
		screen = .setup
	}

	// Handle an invite link such as cheech://join?host=192.168.1.5&port=34567
	// (also accepts cheech://192.168.1.5:34567) by joining that game.
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
		lastMoveSerial = session.moveSerial
		animator.stop()
		animatingPlayer = 0
		lastTapHole = -1
		lastTapWasTerminal = false
		messages.removeAll()
		let seatSpecs: [CheechSeat] = seats.map { seat in
			switch seat.kind {
			case .human:
				return CheechSeat.human(withName: seat.name, color: seat.color)
			case .computer:
				return CheechSeat.computer(withType: seat.botType, name: seat.name, color: seat.color)
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
			stopOthers: stopOthers,
			seats: seatSpecs
		)
		screen = .game
	}

	func updateProfile(name: String, color: Int) {
		playerName = name
		playerColor = color
		session.changeName(name)
		session.changeColor(color)
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
