import SwiftUI
import UIKit

struct RootView: View {
	@EnvironmentObject var model: SessionModel

	var body: some View {
		switch model.screen {
		case .setup: GameSetupView()
		case .game: GameScreenView()
		}
	}
}

struct ColorPickerRow: View {
	@Binding var selection: Int

	var body: some View {
		HStack(spacing: 10) {
			ForEach(1..<9, id: \.self) { color in
				Circle()
					.fill(PegColor.swiftUIColor(color)
						.opacity(selection == color ? 1.0 : 0.7))
					.frame(width: 34, height: 34)
					.overlay(
						Circle().stroke(
							selection == color ? Color.primary
								: Color.secondary.opacity(0.7),
							lineWidth: selection == color ? 3 : 1)
					)
					.onTapGesture { selection = color }
			}
		}
	}
}

// Player count and rule toggles, shared by the pre-game setup page and the
// in-game setup sheet (which binds to a draft rather than the live prefs).
struct RulesRows: View {
	@Binding var numPlayers: Int
	@Binding var longJumps: Bool
	@Binding var hopOthers: Bool
	@Binding var stopOthers: Bool

	var body: some View {
		Stepper("Players: \(numPlayers)", value: $numPlayers, in: 2...6)
		Toggle("Allow long jumps", isOn: $longJumps)
		Toggle("Can Enter Opponents' Goal", isOn: $hopOthers)
		if hopOthers {
			Toggle("Can Stop in Opponents' Goal", isOn: $stopOthers)
		}
	}
}

struct GameSetupView: View {
	@EnvironmentObject var model: SessionModel

	private var speedLabel: String {
		let ms = model.opponentStepMs
		return ms == 0 ? "Instant" : "\(ms) ms"
	}

	var body: some View {
		VStack(spacing: 0) {
			HStack(alignment: .center) {
				VStack(alignment: .leading, spacing: 2) {
					Text("Cheech")
						.font(.system(size: 44, weight: .bold, design: .rounded))
						.lineLimit(1)
						.minimumScaleFactor(0.6)
					Text("Chinese Checkers")
						.font(.headline)
						.foregroundStyle(.secondary)
				}
				Spacer(minLength: 12)
				Button {
					if model.setupMode == .start {
						model.startGame()
					} else {
						model.joinGame(spectator: model.spectator)
					}
				} label: {
					Text(model.setupMode == .start ? "Start Game" : "Join Game")
						.font(.headline)
				}
				.buttonStyle(.borderedProminent)
				.tint(.green)
				.controlSize(.large)
			}
			.padding(.horizontal, 20)
			.padding(.top, 20)
			.padding(.bottom, 6)

			Form {
				Section {
					Picker("Mode", selection: $model.setupMode) {
						Text("Start a Game").tag(SetupMode.start)
						Text("Join a Game").tag(SetupMode.join)
					}
					.pickerStyle(.segmented)
					.labelsHidden()
				}

				if model.setupMode == .start {
					Section("Game Settings") {
						RulesRows(
							numPlayers: $model.numPlayers,
							longJumps: $model.longJumps,
							hopOthers: $model.hopOthers,
							stopOthers: $model.stopOthers
						)
						VStack(alignment: .leading, spacing: 6) {
							HStack {
								Text("Animation Speed")
								Spacer()
								Text(speedLabel)
									.foregroundStyle(.secondary)
							}
							Slider(value: Binding(
								get: { Double(model.opponentStepMs) },
								set: { model.opponentStepMs = Int($0.rounded()) }
							), in: 0...500, step: 10)
						}
						.padding(.vertical, 2)
					}
					Section("Players") {
						ForEach($model.seats) { $seat in
							SeatEditorView(seat: $seat)
						}
					}
					if model.seats.contains(where: { $0.kind == .remote }) {
						Section {
							Button("Invite a Friend") { model.showInvite = true }
								.frame(maxWidth: .infinity)
						}
					}
				} else {
					Section("Server") {
						HStack {
							Text("Host")
							TextField("Host", text: $model.joinHost)
								.autocorrectionDisabled()
								.textInputAutocapitalization(.never)
								.multilineTextAlignment(.trailing)
						}
						HStack {
							Text("Port")
							TextField("Port", value: $model.joinPort, format: .number.grouping(.never))
								.keyboardType(.numberPad)
								.multilineTextAlignment(.trailing)
						}
					}
					Section("You") {
						HStack {
							Text("Name")
							TextField("Name", text: $model.playerName)
								.multilineTextAlignment(.trailing)
						}
						if !model.spectator {
							ColorPickerRow(selection: $model.playerColor)
						}
						Toggle("Join as spectator", isOn: $model.spectator)
					}
				}
			}
		}
		.sheet(isPresented: $model.showInvite) {
			InviteView().environmentObject(model)
		}
	}
}

struct InviteView: View {
	@EnvironmentObject var model: SessionModel

	private var isHosting: Bool { model.session.isHost }
	// When hosting, share this device's Wi-Fi address.  When joined, share the
	// address this client connected to (the server), unless it is loopback in
	// which case fall back to the local address.
	private var host: String {
		let connected = model.session.serverHost
		let lower = connected.lowercased()
		if !connected.isEmpty && lower != "127.0.0.1" && lower != "localhost" {
			return connected
		}
		return localIPAddress() ?? "Unavailable"
	}
	private var port: Int {
		let serverPort = Int(model.session.serverPort)
		return serverPort > 0 ? serverPort : model.hostPort
	}
	private var canInvite: Bool { host != "Unavailable" }
	private var inviteURL: String {
		var components = URLComponents()
		components.scheme = "cheech"
		components.host = "join"
		components.queryItems = [
			URLQueryItem(name: "host", value: host),
			URLQueryItem(name: "port", value: String(port)),
		]
		return components.string ?? "cheech://join?host=\(host)&port=\(port)"
	}

	var body: some View {
		NavigationStack {
			Form {
				Section("Share with friends") {
					HStack {
						Text("Host")
						Spacer()
						Text(host).foregroundStyle(.secondary)
					}
					if isHosting {
						HStack {
							Text("Port")
							Spacer()
							TextField("Port", value: $model.hostPort, format: .number.grouping(.never))
								.keyboardType(.numberPad)
								.multilineTextAlignment(.trailing)
						}
					} else {
						HStack {
							Text("Port")
							Spacer()
							Text(String(port)).foregroundStyle(.secondary)
						}
					}
				}
				if canInvite {
					Section("Invite link") {
						Text(inviteURL)
							.font(.footnote)
							.textSelection(.enabled)
							.foregroundStyle(.secondary)
					}
					Section {
						ShareLink(item: inviteURL) {
							Text("Share Invite").frame(maxWidth: .infinity)
						}
						Button("Copy Invite Link") {
							UIPasteboard.general.string = inviteURL
						}
						.frame(maxWidth: .infinity)
						.listRowSeparator(.hidden, edges: .top)
					} footer: {
						Text("Opening the link joins this game directly.")
					}
				} else {
					Section {
						Text("No local network address found. Connect to Wi-Fi to invite friends.")
							.font(.footnote)
							.foregroundStyle(.secondary)
					}
				}
			}
			.navigationTitle("Invite a Friend")
			.toolbar {
				ToolbarItem(placement: .confirmationAction) {
					Button("Done") { model.showInvite = false }
				}
			}
		}
	}
}

struct BotTypePickerRow: View {
	let title: String
	@Binding var selection: String

	var body: some View {
		HStack {
			Text(title)
			Spacer(minLength: 8)
			DropdownPicker(
				options: BotCatalog.types,
				selection: $selection,
				trigger: { BotCatalog.familyName($0) },
				rowLabel: { BotCatalog.familyName($0) },
				rowDetail: { _ in "" }
			)
		}
	}
}

// Advanced bot settings behind the gear button on a computer seat row: the
// family blurb, search depth, skill detune, and (for Friendly/Mean bots in
// games with more than two players) the focus list.
struct BotDetailsPopover: View {
	@Binding var seat: SeatConfig
	// The other seats, for the focus picker (empty hides it).
	let otherSeats: [SeatConfig]
	let focusLabel: (SeatConfig) -> String

	private var family: String { BotCatalog.familyName(seat.botType) }

	// SwiftUI snapshots popover content while it is presented, so the whole
	// content is keyed on the family: changing bot type rebuilds it instead
	// of leaving a stale blurb and focus list behind.
	private var blurb: String {
		switch family {
		case "Friendly":
			return "Friendly bots help set up good moves for the players marked below."
		case "Mean":
			return "Mean bots try to block the moves of the players marked below."
		default:
			return "Neutral bots just play their own game and leave everyone else alone."
		}
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 14) {
			Text(blurb)
				.font(.callout)
				.multilineTextAlignment(.leading)
				.fixedSize(horizontal: false, vertical: true)
			DepthPickerRow(title: "Depth", selection: $seat.depth)
			SkillPickerRow(title: "Skill", botType: seat.botType, selection: Binding(
				get: { BotSkill.level(seat.skill) },
				set: { seat.skill = $0 }
			))
			if !otherSeats.isEmpty && (family == "Friendly" || family == "Mean") {
				FocusPicker(
					title: family == "Friendly" ? "Friendly to" : "Mean to",
					items: otherSeats,
					id: \.id,
					label: focusLabel,
					selection: family == "Friendly"
						? $seat.friendSeats : $seat.enemySeats
				)
			}
		}
		.padding()
		.frame(width: 320, alignment: .leading)
		.presentationCompactAdaptation(.popover)
		.id(family)
	}
}

// A custom dropdown, since a Menu cannot style its option rows (and a Picker
// wheel needs a whole new presentation).  The trigger shows the selected
// option's trigger text; the expanded list shows each option's label (leading,
// blue) and detail (trailing, dark grey).
struct DropdownPicker<Option: Hashable>: View {
	let options: [Option]
	@Binding var selection: Option
	let trigger: (Option) -> String
	let rowLabel: (Option) -> String
	let rowDetail: (Option) -> String
	@StateObject private var state = PopoverState()

	var body: some View {
		Button {
			state.isVisible.toggle()
		} label: {
			HStack(spacing: 4) {
				Text(trigger(selection))
					.lineLimit(1)
					.minimumScaleFactor(0.7)
				Image(systemName: "chevron.up.chevron.down")
					.font(.caption2.weight(.semibold))
					.foregroundStyle(.tertiary)
			}
		}
		.buttonStyle(.plain)
		.popover(isPresented: $state.isVisible) {
			VStack(spacing: 0) {
				ForEach(options, id: \.self) { option in
					Button {
						selection = option
						state.isVisible = false
					} label: {
						HStack(spacing: 16) {
							Text(rowLabel(option))
								.foregroundStyle(.blue)
								.lineLimit(1)
							Spacer(minLength: 24)
							Text(rowDetail(option))
								.foregroundStyle(Color(.darkGray))
								.lineLimit(1)
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 16)
						.contentShape(Rectangle())
					}
					.buttonStyle(.plain)
					if option != options.last { Divider() }
				}
			}
			.padding(.vertical, 5)
			.fixedSize(horizontal: true, vertical: false)
			.presentationCompactAdaptation(.popover)
		}
	}
}

// Backing for a transient popover shown inline in a form row.  (Held as an
// ObservableObject because this toolchain cannot expand @State.)
final class PopoverState: ObservableObject {
	@Published var isVisible = false
}

struct DepthPickerRow: View {
	let title: String
	@Binding var selection: Int?

	var body: some View {
		HStack {
			Text(title)
			Spacer(minLength: 8)
			DropdownPicker(
				options: BotSkill.depthOptions,
				selection: $selection,
				trigger: { BotSkill.depthLabel(for: $0) },
				rowLabel: { BotSkill.depthLabel(for: $0) },
				rowDetail: { $0 == nil ? "Family default" : "" }
			)
		}
	}
}

struct SkillPickerRow: View {
	let title: String
	let botType: String
	@Binding var selection: Int

	var body: some View {
		HStack {
			Text(title)
			Spacer(minLength: 8)
			DropdownPicker(
				options: BotSkill.levels,
				selection: $selection,
				trigger: { BotSkill.label(for: $0) },
				rowLabel: { BotSkill.label(for: $0) },
				rowDetail: { BotSkill.detail(for: $0, type: botType) }
			)
		}
	}
}

// A checkbox list (two items per row to keep the setup table compact) used to
// choose which players a Friendly bot helps or a Mean bot opposes.  A nil
// selection means "everyone" and shows as all boxes checked; toggling
// materialises an explicit selection (an empty one means "nobody").  Used on
// the setup page over seat ids and in-game over player numbers.
struct FocusPicker<Item, ID: Hashable>: View {
	let title: String
	let items: [Item]
	// An item's stable id: seat ids on the setup page, player numbers in-game.
	let id: KeyPath<Item, ID>
	let label: (Item) -> String
	@Binding var selection: [ID]?

	private var chosen: Set<ID> {
		Set(selection ?? items.map { $0[keyPath: id] })
	}

	private func toggle(_ target: ID) {
		var set = chosen
		if set.contains(target) { set.remove(target) } else { set.insert(target) }
		selection = items.map { $0[keyPath: id] }.filter { set.contains($0) }
	}

	// Two items per row to keep the setup table compact.
	private var rows: [[Item]] {
		stride(from: 0, to: items.count, by: 2).map {
			Array(items[$0..<min($0 + 2, items.count)])
		}
	}

	// Name right-aligned against the switch so the two read as one control.
	private func focusToggle(_ name: String, isOn: Binding<Bool>) -> some View {
		HStack(spacing: 6) {
			Text(name)
				.lineLimit(1)
				.frame(maxWidth: .infinity, alignment: .trailing)
			Toggle("", isOn: isOn)
				.labelsHidden()
		}
		.frame(maxWidth: .infinity)
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 6) {
			Text(title)
				.font(.subheadline)
				.foregroundStyle(.secondary)
			ForEach(Array(rows.enumerated()), id: \.offset) { _, row in
				HStack(spacing: 12) {
					ForEach(row, id: id) { item in
						focusToggle(label(item), isOn: Binding(
							get: { chosen.contains(item[keyPath: id]) },
							set: { _ in toggle(item[keyPath: id]) }
						))
					}
					if row.count == 1 {
						Color.clear.frame(maxWidth: .infinity)
					}
				}
			}
		}
	}
}

struct SeatEditorView: View {
	@EnvironmentObject var model: SessionModel
	@Binding var seat: SeatConfig
	// Backing for the advanced-settings popover.  (Held as an
	// ObservableObject because this toolchain cannot expand @State.)
	@StateObject private var details = PopoverState()

	// Picking a color another seat already uses swaps the two, so no two
	// seats share a color.
	private var colorBinding: Binding<Int> {
		Binding(
			get: { seat.color },
			set: { model.setColor(forSeat: seat.id, to: $0) }
		)
	}

	// How another seat reads in a focus picker: humans by name, bots by
	// family ("Neutral", not a cute name), remote as an open seat.
	private func otherLabel(_ other: SeatConfig) -> String {
		switch other.kind {
		case .human:
			return other.name.isEmpty ? "Seat" : other.name
		case .computer:
			return BotCatalog.familyName(other.botType)
		case .remote:
			return "Open seat"
		}
	}

	var body: some View {
		VStack(alignment: .leading, spacing: 16) {
			Picker("Type", selection: Binding(
				get: { seat.kind },
				set: { newKind in
					seat.kind = newKind
					switch newKind {
					case .computer:
						if seat.humanName == nil { seat.humanName = seat.name }
						seat.name = BotCatalog.cuteName(seat.botType)
					case .human:
						seat.name = seat.humanName ?? seat.name
					default:
						break
					}
				}
			)) {
				Text("Human").tag(SeatKind.human)
				Text("Computer").tag(SeatKind.computer)
				Text("Remote").tag(SeatKind.remote)
			}
			.pickerStyle(.segmented)

			if seat.kind != .remote {
				ColorPickerRow(selection: colorBinding)
			}

			switch seat.kind {
			case .human:
				HStack {
					Text("Name")
					TextField("Name", text: Binding(
						get: { seat.name },
						set: { newName in
							seat.name = newName
							seat.humanName = newName
						}
					))
					.multilineTextAlignment(.trailing)
				}
			case .computer:
				HStack(spacing: 16) {
					BotTypePickerRow(title: "Bot Type", selection: Binding(
						get: { seat.botType },
						set: { newType in
							seat.botType = newType
							model.lastBotType = newType
							seat.name = BotCatalog.cuteName(newType)
						}
					))
					// Advanced settings (depth, skill, focus) live in this
					// popover so the seat row stays a single line.
					Button {
						details.isVisible.toggle()
					} label: {
						Image(systemName: "slider.horizontal.3")
							.foregroundStyle(.secondary)
					}
					.buttonStyle(.plain)
					.popover(isPresented: $details.isVisible) {
						BotDetailsPopover(
							seat: $seat,
							otherSeats: model.seats.count > 2
								? model.seats.filter { $0.id != seat.id } : [],
							focusLabel: { otherLabel($0) }
						)
					}
				}
			case .remote:
				Text("Open seat — another device can join")
					.font(.caption)
					.foregroundStyle(.secondary)
			}
		}
		.padding(.vertical, 4)
	}
}

struct GameScreenView: View {
	@EnvironmentObject var model: SessionModel
	@Environment(\.verticalSizeClass) private var verticalSizeClass

	private var session: CheechSession { model.session }

	// True while the server still has empty seats, so an invite is useful.
	private var hasOpenSpots: Bool {
		session.connected && Int(session.numPlayers) > 0
			&& Int(session.playerCount) < Int(session.numPlayers)
			&& session.status != .won && session.status != .end
	}

	private var rotation: Double {
		let player: Int
		if session.isHost {
			// A lone local human (rest computer/remote seats) is placed at the
			// bottom, matching the desktop app.  With several local humans
			// (hotseat) keep the fixed orientation.
			let local = Int(session.localHumanPlayerNumber)
			player = local > 0 ? local : 1
		} else {
			player = Int(session.myPlayerNumber)
		}
		return BoardGeometry.rotation(numPlayers: Int(session.numPlayers), player: player)
	}

	var body: some View {
		Group {
			if model.fullScreen {
				fullScreenBody
			} else {
				normalBody
			}
		}
		.background(keyboardShortcuts)
		.confirmationDialog(
			model.confirmAction.map(confirmTitle) ?? "Are you sure?",
			isPresented: $model.showConfirm,
			presenting: model.confirmAction
		) { action in
			Button(confirmButtonTitle(action), role: .destructive) {
				model.showConfirm = false
				model.confirmAction = nil
				execute(action)
			}
			Button("Cancel", role: .cancel) {
				model.showConfirm = false
				model.confirmAction = nil
			}
		} message: { _ in
			Text("The current game will be lost.")
		}
		.onChange(of: model.showConfirm) { _, showing in
			if !showing { model.confirmAction = nil }
		}
		.sheet(isPresented: $model.showProfile) {
			ProfileEditorView()
				.environmentObject(model)
		}
		.sheet(isPresented: $model.showInvite) {
			InviteView()
				.environmentObject(model)
		}
		.sheet(isPresented: $model.showGameSetup) {
			GameSetupSheet()
				.environmentObject(model)
		}
	}

	// Actions that discard the current game ask for confirmation once any move
	// has been played; before the first move, or once the game has finished,
	// they run straight away.
	private func perform(_ action: GameAction) {
		if session.moveNumber > 0 && !model.allPlayersFinished {
			model.confirmAction = action
			model.showConfirm = true
		} else {
			execute(action)
		}
	}

	private func execute(_ action: GameAction) {
		switch action {
		case .leave: model.leave()
		case .restart: session.restartGame()
		case .rotate: session.rotatePlayers()
		case .shuffle: session.shufflePlayers()
		}
	}

	private func confirmTitle(_ action: GameAction) -> String {
		switch action {
		case .leave: return "Leave the game?"
		case .restart: return "Restart the game?"
		case .rotate: return "Rotate the players?"
		case .shuffle: return "Shuffle the players?"
		}
	}

	private func confirmButtonTitle(_ action: GameAction) -> String {
		switch action {
		case .leave: return "Leave"
		case .restart: return "Restart"
		case .rotate: return "Rotate"
		case .shuffle: return "Shuffle"
		}
	}

	// Label for how this device is taking part, shown bottom-left, with the
	// server-log toggle beside it.
	private var roleLabel: String {
		if session.isHost { return "Hosting" }
		if session.isSpectator { return "Spectator" }
		return "Joined"
	}

	private var normalBody: some View {
		VStack(spacing: 8) {
			HStack {
				Button("Close") { perform(.leave) }
				Spacer()
				if !session.isHost && !session.isSpectator {
					Button { model.showProfile = true } label: {
						HStack(spacing: 4) {
							Circle()
								.fill(PegColor.swiftUIColor(myColor))
								.frame(width: 12, height: 12)
							Text(myName).font(.subheadline)
							Image(systemName: "pencil").font(.caption2)
						}
					}
					.foregroundStyle(.secondary)
					.padding(.trailing, 8)
				}
				if session.connected {
					Menu {
						if session.isHost && session.hasLocalHumanSeat {
							Button("Undo Move") { session.undoMove() }
							Button("Restart Game") { perform(.restart) }
							Button("Rotate Players") { perform(.rotate) }
							Button("Shuffle Players") { perform(.shuffle) }
							Divider()
						}
						Button("Setup Game") { model.showGameSetup = true }
						if hasOpenSpots {
							Button("Invite More Players") { model.showInvite = true }
						}
					} label: {
						Image(systemName: "ellipsis.circle")
					}
					.padding(.trailing, 8)
				}
				Button {
					model.fullScreen = true
				} label: {
					Image(systemName: "arrow.up.left.and.arrow.down.right")
				}
				.accessibilityLabel("Full Screen")
			}
			.padding(.horizontal)

			playerList
				.padding(.top, verticalSizeClass == .regular ? 6 : 0)

			let round = moveRound
			let showRound = session.connected && session.status != .waiting
			Text(showRound ? "Move \(round)" : " ")
				.font(.system(size: 16, weight: .bold))
				.foregroundStyle(.secondary)

			BoardView(
				model: model,
				animator: model.animator,
				rotation: rotation,
				leadingOverlay: AnyView(turnBadge),
				bottomTrailingOverlay: AnyView(moveControls)
			)
			.frame(maxWidth: .infinity, maxHeight: .infinity)

			if model.showServerLog {
				messageStrip
			}
		}
		.padding(.vertical, 8)
		.overlay(alignment: .bottomLeading) {
			if model.canShowServerLog {
				HStack(spacing: 6) {
					Text(roleLabel)
						.font(.subheadline)
						.foregroundStyle(.secondary)
					Button {
						model.showServerLog.toggle()
					} label: {
						Image(systemName: model.showServerLog ? "xmark.circle" : "info.circle")
					}
					.accessibilityLabel(model.showServerLog ? "Hide Server Log" : "Show Server Log")
				}
				.padding(.leading)
			}
		}
	}

	private var fullScreenBody: some View {
		ZStack(alignment: .topTrailing) {
			BoardView(
				model: model,
				animator: model.animator,
				rotation: rotation,
				leadingOverlay: AnyView(turnBadge),
				bottomTrailingOverlay: AnyView(moveControls)
			)
			.frame(maxWidth: .infinity, maxHeight: .infinity)
			.background(Color.black.ignoresSafeArea())
			.ignoresSafeArea()

			fullScreenExit
				.padding(.trailing)
				.padding(.top, 4)
		}
	}

	// Move/Cancel for the in-progress path, pinned to the bottom-right corner
	// of the board (which has no holes).
	@ViewBuilder private var moveControls: some View {
		let canControl = !session.isSpectator && (!session.isHost || session.activeSeatKind == .human)
		let selectedCount = session.selectedHoles().count
		if selectedCount > 0 && canControl {
			HStack(spacing: 12) {
				Button("Cancel") { session.clearSelection() }
					.buttonStyle(.bordered)
				Button("Move") { session.confirmMove() }
					.buttonStyle(.borderedProminent)
					.disabled(selectedCount < 2)
			}
		}
	}

	// Who's turn it is, pinned to the board corner (which has no holes).
	private var turnBadge: some View {
		VStack(alignment: .leading, spacing: 8) {
			HStack(spacing: 6) {
				Circle()
					.fill(PegColor.swiftUIColor(badgeColor))
					.frame(width: 12, height: 12)
				Text(turnLabel)
					.font(.caption)
					.fontWeight(.semibold)
					.lineLimit(1)
					.minimumScaleFactor(0.7)
			}
			.foregroundStyle(.white)
			.padding(.horizontal, 10)
			.padding(.vertical, 6)
			.background(.ultraThinMaterial, in: Capsule())

			if winnerText != nil && session.isHost {
				Button {
					session.restartGame()
				} label: {
					Text("Play Again")
						.font(.caption)
						.fontWeight(.semibold)
				}
				.buttonStyle(.borderedProminent)
				.tint(.green)
				.controlSize(.small)
			}
		}
	}

	// Whose turn it is according to what is actually happening on the board:
	// while a move replays, the player who made it is still shown, even though
	// the server has already handed the turn to the next player.
	private var displayedPlayer: Int {
		model.animatingPlayer != 0 ? model.animatingPlayer : Int(session.currentPlayer)
	}

	// The winner, once everyone has finished.  The first player to get all pegs
	// home is the one with the fewest recorded moves.
	private var winnerPlayer: Int? {
		guard model.allPlayersFinished else { return nil }
		let finished = (1...Int(session.numPlayers)).filter { session.finished(forPlayer: $0) }
		return finished.min(by: { a, b in
			let ma = Int(session.finishedInMoves(forPlayer: a))
			let mb = Int(session.finishedInMoves(forPlayer: b))
			let na = ma > 0 ? ma : Int.max
			let nb = mb > 0 ? mb : Int.max
			return na == nb ? a < b : na < nb
		})
	}

	private var winnerText: String? {
		guard model.allPlayersFinished else { return nil }
		guard let winner = winnerPlayer else { return "Game over" }
		let name = session.name(forPlayer: winner)
		return name.isEmpty ? "Player \(winner) won!" : "\(name) won!"
	}

	private var turnLabel: String {
		if let winner = winnerText { return winner }
		if session.status == .waiting {
			return "Waiting for players"
		}
		let p = displayedPlayer
		if session.isHost {
			let name = session.name(forPlayer: p)
			return name.isEmpty ? "Player \(p)'s turn" : "\(name)'s turn"
		}
		if session.isSpectator { return "Player \(p)'s turn" }
		if p == Int(session.myPlayerNumber) { return "Your turn" }
		let name = session.name(forPlayer: p)
		return name.isEmpty ? "Player \(p)'s turn" : "\(name)'s turn"
	}

	private var fullScreenExit: some View {
		Button {
			model.fullScreen = false
		} label: {
			Image(systemName: "arrow.down.right.and.arrow.up.left")
				.font(.body.weight(.semibold))
				.padding(8)
				.background(.ultraThinMaterial, in: Circle())
		}
		.accessibilityLabel("Exit Full Screen")
	}

	private var currentPlayerColor: Int {
		let p = displayedPlayer
		let c = p > 0 ? Int(session.color(forPlayer: p)) : 0
		return c > 0 ? c : model.playerColor
	}

	// The dot beside the badge: the winner's color once the game is over,
	// otherwise the current player's.
	private var badgeColor: Int {
		if let winner = winnerPlayer {
			let c = Int(session.color(forPlayer: winner))
			if c > 0 { return c }
		}
		return currentPlayerColor
	}

	private var myName: String {
		let p = Int(session.myPlayerNumber)
		let n = p > 0 ? session.name(forPlayer: p) : ""
		return n.isEmpty ? model.playerName : n
	}

	private var myColor: Int {
		let p = Int(session.myPlayerNumber)
		let c = p > 0 ? Int(session.color(forPlayer: p)) : 0
		return c > 0 ? c : model.playerColor
	}

	// The number of completed moves for the furthest-along player.  This is the
	// real per-player count, unlike total moves divided by the number of players
	// (which undercounts when one player keeps moving after the others finish).
	private var moveRound: Int {
		let players = max(Int(session.numPlayers), 1)
		var movesMade = 0
		for p in 1...players {
			let own = Int(session.movesTaken(forPlayer: p))
			let finished = Int(session.finishedInMoves(forPlayer: p))
			movesMade = max(movesMade, max(own, finished))
		}
		return max(movesMade, 1)
	}

	private var playerList: some View {
		GeometryReader { geo in
			ScrollView(.horizontal, showsIndicators: false) {
				HStack(spacing: 14) {
					ForEach(1...6, id: \.self) { p in
						let name = session.name(forPlayer: p)
						if !name.isEmpty {
							HStack(spacing: 5) {
								Circle()
									.fill(PegColor.swiftUIColor(Int(session.color(forPlayer: p))))
									.frame(width: 14, height: 14)
								Text(name).font(.system(size: 16))
								if session.finished(forPlayer: p) {
									let own = Int(session.movesTaken(forPlayer: p))
									let moves = own > 0 ? own : Int(session.finishedInMoves(forPlayer: p))
									if moves > 0 {
										Text("(\(moves))").font(.caption)
									}
//									Image(systemName: "checkmark.seal.fill").font(.caption2)
								}
							}
							.opacity(Int(session.currentPlayer) == p ? 1 : 0.6)
						}
					}
				}
				.padding(.horizontal)
				.frame(minWidth: geo.size.width, alignment: .center)
			}
		}
		.frame(height: 24)
	}

	private var messageStrip: some View {
		let recent = Array(model.messages.suffix(3))
		return VStack(alignment: .leading, spacing: 2) {
			ForEach(Array(recent.enumerated()), id: \.offset) { _, message in
				Text(message)
					.font(.caption)
					.foregroundStyle(.secondary)
					.frame(maxWidth: .infinity, alignment: .leading)
			}
		}
		.padding(.horizontal)
		.frame(height: 52, alignment: .topLeading)
	}

	private var keyboardShortcuts: some View {
		Group {
			Button("Move") { session.confirmMove() }
				.keyboardShortcut(.return, modifiers: [])
			Button("Cancel") { session.clearSelection() }
				.keyboardShortcut(.escape, modifiers: [])
			Button("Remove Hop") { model.removeLastHop() }
				.keyboardShortcut(.delete, modifiers: [])
			Button("Remove Hop") { model.removeLastHop() }
				.keyboardShortcut(.deleteForward, modifiers: [])
		}
		.frame(width: 0, height: 0)
		.opacity(0)
		.accessibilityHidden(true)
	}
}

// Editable copy of the profile, so edits can be abandoned with "Cancel"
// without touching the live player name/color.  (Held as an ObservableObject
// because this toolchain cannot expand @State.)
final class ProfileDraft: ObservableObject {
	@Published var name = ""
	@Published var color = 1
}

struct ProfileEditorView: View {
	@EnvironmentObject var model: SessionModel
	@StateObject private var draft = ProfileDraft()

	var body: some View {
		NavigationStack {
			Form {
				Section("Name") {
					HStack {
						Text("Name")
						TextField("Name", text: $draft.name)
							.multilineTextAlignment(.trailing)
					}
				}
				Section("Color") {
					ColorPickerRow(selection: $draft.color)
				}
			}
			.navigationTitle("You")
			.toolbar {
				ToolbarItem(placement: .cancellationAction) {
					Button("Cancel") { model.showProfile = false }
				}
				ToolbarItem(placement: .confirmationAction) {
					Button("Done") {
						model.updateProfile(name: draft.name, color: draft.color)
						model.showProfile = false
					}
				}
			}
			.onAppear {
				draft.name = model.playerName
				draft.color = model.playerColor
			}
		}
	}
}

// Editable copy of the in-game setup, so the form can be changed without
// touching the running game until "Apply" is tapped.  (Held as an
// ObservableObject because this toolchain cannot expand @State.)
final class GameSetupDraft: ObservableObject {
	@Published var numPlayers = 3
	@Published var longJumps = false
	@Published var hopOthers = true
	@Published var stopOthers = true
	@Published var botType = "LookAhead(3)"
	// Which players a Friendly/Mean bot focuses on, as current player numbers
	// (the other players are already seated, so there are no seat ids to pick
	// from).  nil means "everyone"; an empty list means "nobody".
	@Published var friendPlayers: [Int]?
	@Published var enemyPlayers: [Int]?
	@Published var confirmApply = false
}

// In-game setup for a hosted or joined game: change the player count and rules
// (which restarts the board) and add/remove computer players that connect to
// the same host as this client.
struct GameSetupSheet: View {
	@EnvironmentObject var model: SessionModel
	@StateObject private var draft = GameSetupDraft()

	private var session: CheechSession { model.session }

	private var canAddComputer: Bool {
		Int(session.playerCount) < draft.numPlayers
	}

	// Player numbers currently seated, 1...playerCount, for the focus pickers.
	private var playerNumbers: [Int] {
		let count = Int(session.playerCount)
		return count > 0 ? Array(1...count) : []
	}

	private var focusFamily: String {
		BotCatalog.familyName(draft.botType)
	}

	private func addComputerPlayer() {
		model.lastBotType = draft.botType
		var friends: [Int]?
		var enemies: [Int]?
		if !playerNumbers.isEmpty {
			if focusFamily == "Friendly" {
				friends = draft.friendPlayers ?? playerNumbers
			} else if focusFamily == "Mean" {
				enemies = draft.enemyPlayers ?? playerNumbers
			}
		}
		// nil friends means "everyone"; the new bot takes the open slot, so
		// it is never in the list and "everyone" means numPlayers - 1 others.
		let friendCount = friends?.count ?? draft.numPlayers - 1
		let botType = BotCatalog.cappedForSingleFriend(
			type: draft.botType, friendCount: friendCount)
		model.addComputerPlayer(
			type: botType, name: BotCatalog.cuteName(botType),
			color: 0,
			friendPlayers: friends, enemyPlayers: enemies
		)
	}

	// The name a seated player reads in a focus picker.
	private func playerLabel(_ player: Int) -> String {
		let name = session.name(forPlayer: player)
		return name.isEmpty ? "Player \(player)" : name
	}

	// Whether the draft differs from the running game, so "Apply" has work to
	// do (applying restarts the game).
	private var hasChanges: Bool {
		draft.numPlayers != Int(session.numPlayers)
			|| draft.longJumps != session.longJumps
			|| draft.hopOthers != session.hopOthers
			|| (draft.hopOthers && draft.stopOthers != session.stopOthers)
	}

	// Applying restarts the game, so only confirm once any move has been played
	// and the game is not already over.
	private var gameInProgress: Bool {
		session.moveNumber > 0 && !model.allPlayersFinished
	}

	private func apply() {
		model.reconfigureGame(
			numPlayers: draft.numPlayers,
			longJumps: draft.longJumps,
			hopOthers: draft.hopOthers,
			stopOthers: draft.stopOthers
		)
		model.showGameSetup = false
	}

	var body: some View {
		NavigationStack {
			Form {
				Section("Players") {
					RulesRows(
						numPlayers: $draft.numPlayers,
						longJumps: $draft.longJumps,
						hopOthers: $draft.hopOthers,
						stopOthers: $draft.stopOthers
					)
					HStack {
						Text("Connected")
						Spacer()
						Text("\(session.playerCount)")
							.foregroundStyle(.secondary)
					}
				}

				Section("Computer Players") {
					BotTypePickerRow(title: "Type", selection: $draft.botType)
					if focusFamily == "Friendly" && draft.numPlayers > 2 && !playerNumbers.isEmpty {
						FocusPicker(
							title: "Friendly to",
							items: playerNumbers,
							id: \.self,
							label: { playerLabel($0) },
							selection: $draft.friendPlayers
						)
					} else if focusFamily == "Mean" && draft.numPlayers > 2 && !playerNumbers.isEmpty {
						FocusPicker(
							title: "Mean to",
							items: playerNumbers,
							id: \.self,
							label: { playerLabel($0) },
							selection: $draft.enemyPlayers
						)
					}
					Button("Add Computer Player") {
						addComputerPlayer()
					}
					.disabled(!canAddComputer)
					Button("Remove All Computer Players", role: .destructive) {
						model.removeComputerPlayers()
					}
					.disabled(Int(session.computerPlayerCount) == 0)
					if Int(session.extraComputerPlayerCount) > 0 {
						HStack {
							Text("Added by you")
							Spacer()
							Text("\(session.extraComputerPlayerCount)")
								.foregroundStyle(.secondary)
						}
					}
				}
			}
			.navigationTitle("Setup Game")
			.toolbar {
				ToolbarItem(placement: .cancellationAction) {
					Button("Cancel") { model.showGameSetup = false }
				}
				ToolbarItem(placement: .confirmationAction) {
					Button("Apply") {
						if gameInProgress {
							draft.confirmApply = true
						} else {
							apply()
						}
					}
					.disabled(!hasChanges)
				}
			}
			.confirmationDialog(
				"Start a new game?",
				isPresented: $draft.confirmApply,
				titleVisibility: .visible
			) {
				Button("Restart Game", role: .destructive) { apply() }
				Button("Cancel", role: .cancel) {}
			} message: {
				Text("Applying these settings restarts the game for all players.")
			}
			.onAppear {
				let players = Int(session.numPlayers)
				draft.numPlayers = min(max(players > 0 ? players : model.numPlayers, 2), 6)
				draft.longJumps = session.longJumps
				draft.hopOthers = session.hopOthers
				draft.stopOthers = session.hopOthers ? session.stopOthers : model.stopOthers
				draft.botType = model.lastBotType
			}
		}
	}
}
