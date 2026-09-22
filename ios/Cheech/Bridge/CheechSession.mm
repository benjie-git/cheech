//
//  CheechSession.mm
//  Cheech
//
//  Objective-C++ bridge around the cheech core.
//

#import "CheechSession.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cheech_loop.hh"
#include "game_server.hh"
#include "game_client.hh"
#include "game_board.hh"
#include "game_hole.hh"
#include "bot_base.hh"
#include "cheech_move_gate.hh"
#include <glibmm/main.h>

namespace
{

NSString *ns(const Glib::ustring &s)
{
	NSString *result = [NSString stringWithUTF8String:s.c_str()];
	return result ? result : @"";
}

void ensureLoopStarted()
{
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		cheech::Loop::instance().start();
	});
}

// Client-side move animation timing defaults.  The bot itself moves
// immediately (a blocked event loop is undesirable on iOS), so the UI replays
// each hop over this duration instead of watching the bot step through the
// move.  The destination rest time is always twice the per-hop step.
const int kAnimateStepMs = 250;
const int kMaxAnimateStepMs = 500;

struct PlayerInfo
{
	std::string name;
	int color = 0;
};

struct Snapshot
{
	bool connected = false;
	bool isHost = false;
	bool isSpectator = false;
	int activeSeatKind = CheechSeatUnassigned;
	bool hasLocalHumanSeat = false;
	bool hasRemoteSeat = false;
	int localHumanPlayerNumber = 0;
	int status = 0;
	int numPlayers = 0;
	int myPlayerNumber = 0;
	int currentPlayer = 0;
	int playerCount = 0;
	std::string serverHost;
	int serverPort = 0;
	bool longJumps = false;
	bool hopOthers = true;
	bool stopOthers = true;
	int extraBotCount = 0;
	// 1-based (index 0 unused).  Always sized so that callers querying players
	// 1..6 never index an empty vector before the first rebuildSnapshot() (the
	// UI can render the game screen before the loop thread populates it).
	std::vector<PlayerInfo> players = std::vector<PlayerInfo>(7);
	bool finished[7] = {false};
	int finishedInMoves[7] = {0};
	int movesTaken[7] = {0};
	int pegsInGoal[7] = {0};
	int holes[GameBoard::SIZE] = {0};
	std::vector<int> selected;
	std::vector<int> lastMove;
	unsigned int moveSerial = 0;
	int moveNumber = 0;
};

struct Seat
{
	CheechSeatKind kind = CheechSeatHuman;
	std::string botType;
	std::string name;
	int color = 0;
	int smarts = 100;
	GameClient *client = nullptr; // when kind == CheechSeatHuman
	BotBase *bot = nullptr;       // when kind == CheechSeatComputer
	int playerNumber = 0;         // assigned after connect
};

struct SessionImpl
{
	GameServer *server = nullptr;
	GameClient *client = nullptr;
	std::vector<BotBase *> bots;

	// Locally hosted seat model.  The display client above is a spectator that
	// mirrors the board; each human seat has its own client, each computer seat
	// has a bot, and each remote seat is left open for another device to join.
	std::vector<Seat> seats;
	std::unordered_map<int, GameClient *> clientByNumber;

	// Computer players added in-game (via -addComputerPlayerOfType:...) on this
	// device.  They connect to whatever host this client is connected to, and
	// are kept separate from the bots that back locally hosted computer seats.
	std::vector<BotBase *> extraBots;

	// Specs for the in-game computer players above (parallel to extraBots), so
	// a fully local game can recreate them when it is restored.
	struct ExtraBotInfo
	{
		std::string type;
		std::string name;
		int color = 0;
		int smarts = 100;
	};
	std::vector<ExtraBotInfo> extraBotInfos;

	// Opaque snapshot of the current fully local game, kept up to date as the
	// board changes so it can be persisted when the app is backgrounded.  Empty
	// whenever the current game is not a restorable all-local game.
	std::string savedState;

	std::vector<unsigned int> selection;
	int configuredPlayers = 0;
	int status = 0;
	bool isHost = false;
	bool isSpectator = false;

	Snapshot snap;
	std::mutex mutex;
	int nameCounter = 0;
	int animStepMs = kAnimateStepMs;
	int animDoneMs = kAnimateStepMs * 2;
	int computerSmarts = 100;
	int hostPort = 0;

	// Hosted games join their seats one at a time so that player numbers are
	// assigned in seat order (connections otherwise race, e.g. seat 2 can be
	// assigned before seat 1).  joinTimer polls until the current seat has been
	// assigned, then joins the next.
	sigc::connection joinTimer;
	int joinWaitTicks = 0;
};

// A fully local game is serialised as text lines.  GameServer parses the keys
// it owns (players/rules/turn/moves/pegs); the bridge owns port/seats/seat.  It
// is only ever produced for hosted games with no remote seats, so a networked
// game is never persisted or restored.
std::string buildLocalSave(SessionImpl *impl)
{
	std::ostringstream out;
	out << "CHEECHSAVE 1\n";
	out << "port " << impl->hostPort << "\n";
	out << "seats " << (impl->seats.size() + impl->extraBotInfos.size()) << "\n";
	for (const Seat &seat : impl->seats)
	{
		out << "seat " << (int)seat.kind << " " << seat.color << " "
			<< seat.smarts << " "
			<< (seat.botType.empty() ? std::string("-") : seat.botType) << " "
			<< seat.name << "\n";
	}
	for (const SessionImpl::ExtraBotInfo &bot : impl->extraBotInfos)
	{
		out << "seat " << (int)CheechSeatComputer << " " << bot.color << " "
			<< bot.smarts << " "
			<< (bot.type.empty() ? std::string("-") : bot.type) << " "
			<< bot.name << "\n";
	}
	out << std::string(impl->server->save_state().c_str());
	return out.str();
}

struct LocalSave
{
	int port = 0;
	int numPlayers = 0;
	bool longJumps = false;
	bool hopOthers = true;
	bool stopOthers = true;
	std::vector<Seat> seats;
	std::string raw;
	bool valid = false;
};

LocalSave parseLocalSave(const std::string &text)
{
	LocalSave save;
	save.raw = text;
	std::istringstream lines(text);
	std::string line;
	while (std::getline(lines, line))
	{
		std::istringstream in(line);
		std::string key;
		in >> key;
		if (key == "port")
			in >> save.port;
		else if (key == "players")
			in >> save.numPlayers;
		else if (key == "rules")
		{
			int lj = 0, ho = 1, so = 1;
			in >> lj >> ho >> so;
			save.longJumps = lj != 0;
			save.hopOthers = ho != 0;
			save.stopOthers = so != 0;
		}
		else if (key == "seat")
		{
			Seat seat;
			int kind = CheechSeatHuman, color = 0, smarts = 100;
			std::string botType;
			in >> kind >> color >> smarts >> botType;
			std::string name;
			std::getline(in, name);
			if (!name.empty() && name[0] == ' ')
				name.erase(0, 1);
			seat.kind = (CheechSeatKind)kind;
			seat.color = color;
			seat.smarts = smarts;
			seat.botType = (botType == "-") ? "" : botType;
			seat.name = name;
			save.seats.push_back(seat);
		}
	}
	save.valid = save.port > 0 && save.port <= 65535
		&& save.numPlayers >= 2 && save.numPlayers <= 6 && !save.seats.empty();
	return save;
}

} // namespace

@interface CheechSession ()
{
	SessionImpl *_impl;
}
- (void)rebuildSnapshot;
- (void)rebuildAndNotify;
- (void)connectDisplaySignals;
- (void)connectSeatSignals:(GameClient *)client index:(int)index;
- (void)joinHostedSeatsFromIndex:(int)index;
- (BOOL)seatIsJoinedAtIndex:(int)index;
- (GameClient *)activeClient;
- (void)stopCore;
- (void)performAction:(void (^)(GameClient *client))block;
- (void)performServerAction:(void (^)(GameServer *server))block;
@end


@implementation CheechSeat

+ (instancetype)humanWithName:(NSString *)name color:(NSInteger)color
{
	CheechSeat *seat = [[self alloc] init];
	seat.kind = CheechSeatHuman;
	seat.botType = @"";
	seat.name = name;
	seat.color = color;
	seat.smarts = 100;
	return seat;
}

+ (instancetype)computerWithType:(NSString *)type
							name:(NSString *)name
						   color:(NSInteger)color
{
	CheechSeat *seat = [[self alloc] init];
	seat.kind = CheechSeatComputer;
	seat.botType = type;
	seat.name = name;
	seat.color = color;
	seat.smarts = 100;
	return seat;
}

+ (instancetype)remoteSeat
{
	CheechSeat *seat = [[self alloc] init];
	seat.kind = CheechSeatRemote;
	seat.botType = @"";
	seat.name = @"";
	seat.color = 0;
	seat.smarts = 100;
	return seat;
}

@end


@implementation CheechSession

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_impl = new SessionImpl();
		ensureLoopStarted();
	}
	return self;
}

- (void)dealloc
{
	// The loop thread owns the core objects.  If it is running, hand the
	// teardown off to it; otherwise clean up directly.
	SessionImpl *impl = _impl;
	_impl = nullptr;

	if (impl && cheech::Loop::instance().running())
		cheech::Loop::instance().post([impl]() { delete impl; });
	else
		delete impl;
}


#pragma mark - Static board geometry

+ (NSInteger)boardHoleCount
{
	return (NSInteger)GameBoard::SIZE;
}

+ (BOOL)boardHolePresent:(NSInteger)hole
{
	if (hole < 0 || hole >= (NSInteger)GameBoard::SIZE) return NO;
	return GameBoard::BOARD_MAP[hole] >= 0;
}

+ (double)boardHoleX:(NSInteger)hole
{
	int col = (int)(hole % GameBoard::SIZE_X);
	int row = (int)(hole / GameBoard::SIZE_X);
	return (double)col + ((row % 2 == 0) ? 0.5 : 0.0);
}

+ (double)boardHoleY:(NSInteger)hole
{
	int row = (int)(hole / GameBoard::SIZE_X);
	return (double)row * (sqrt(3.0) / 2.0);
}

+ (NSString *)defaultNameForComputerType:(NSString *)type
{
	if (!type || type.length == 0) return @"";

	BotBase *bot = BotBase::new_bot_of_type([type UTF8String]);
	if (!bot) return @"";

	Glib::ustring name = bot->get_default_name();
	delete bot;
	return ns(name);
}


+ (NSString *)typeNameForComputerType:(NSString *)type
{
	if (!type || type.length == 0) return @"";

	BotBase *bot = BotBase::new_bot_of_type([type UTF8String]);
	if (!bot) return @"";

	Glib::ustring name = bot->get_type_name();
	delete bot;
	return ns(name);
}


#pragma mark - Setup

- (void)joinHost:(NSString *)host
			port:(uint16_t)port
	   spectator:(BOOL)spectator
	  playerName:(NSString *)playerName
		   color:(NSInteger)color
{
	NSString *name = [playerName copy];
	__weak CheechSession *weakSelf = self;

	cheech::Loop::instance().post([weakSelf, host, port, spectator, name, color]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		[s stopCore];

		SessionImpl *impl = s->_impl;
		impl->isHost = false;
		impl->isSpectator = spectator;
		impl->configuredPlayers = 0;
		impl->status = 0;

		impl->client = new GameClient();
		[s connectDisplaySignals];
		impl->client->change_name([name UTF8String]);
		impl->client->change_color((int)color);
		impl->client->join_game([host UTF8String], (unsigned int)port, spectator);

		[s rebuildAndNotify];
	});
}

- (void)startGameOnPort:(uint16_t)port
			 numPlayers:(NSInteger)numPlayers
			  longJumps:(BOOL)longJumps
			  hopOthers:(BOOL)hopOthers
			 stopOthers:(BOOL)stopOthers
				  seats:(NSArray<CheechSeat *> *)seats
{
	// Snapshot the seat specs into plain C++ so the loop thread never touches
	// Objective-C objects.
	std::vector<Seat> specs;
	specs.reserve(seats.count);
	for (CheechSeat *seat in seats)
	{
		Seat spec;
		spec.kind = seat.kind;
		spec.botType = seat.botType ? [seat.botType UTF8String] : "";
		spec.name = seat.name ? [seat.name UTF8String] : "";
		spec.color = (int)seat.color;
		spec.smarts = (int)seat.smarts;
		specs.push_back(spec);
	}

	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, port, numPlayers, longJumps,
								   hopOthers, stopOthers, specs]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		[s stopCore];

		SessionImpl *impl = s->_impl;
		impl->isHost = true;
		impl->isSpectator = false;
		impl->configuredPlayers = (int)numPlayers;
		impl->status = 0;
		impl->hostPort = (int)port;

		impl->server = new GameServer((unsigned int)port,
									  (unsigned int)numPlayers,
									  longJumps, hopOthers, stopOthers);
		impl->server->new_game();

		// The display client is a spectator (player number 0): it mirrors the
		// board and receives every broadcast, which makes the UI animate all
		// moves regardless of which seat made them.
		impl->client = new GameClient();
		[s connectDisplaySignals];
		impl->client->change_name("Display");
		impl->client->change_color(0);
		impl->client->join_game("127.0.0.1", (unsigned int)port, true);

		impl->seats = specs;
		// Join seats one at a time so the server assigns player numbers in
		// seat order (see joinHostedSeatsFromIndex:).
		[s joinHostedSeatsFromIndex:0];

		[s rebuildAndNotify];
	});
}

// Resumes a fully local game captured earlier with -localGameSave:.  The
// in-process server and its seats are recreated and the saved board and turn
// are restored before the seats connect, so every client receives the restored
// position through the normal GAME_BOARD sync.
- (BOOL)resumeLocalGame:(NSString *)save
{
	if (save.length == 0) return NO;
	LocalSave parsed = parseLocalSave(std::string([save UTF8String]));
	if (!parsed.valid) return NO;

	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, parsed]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		[s stopCore];

		SessionImpl *impl = s->_impl;
		impl->isHost = true;
		impl->isSpectator = false;
		impl->configuredPlayers = parsed.numPlayers;
		impl->status = 0;
		impl->hostPort = parsed.port;

		impl->server = new GameServer((unsigned int)parsed.port,
									  (unsigned int)parsed.numPlayers,
									  parsed.longJumps, parsed.hopOthers,
									  parsed.stopOthers);
		impl->server->new_game();
		impl->server->load_state(Glib::ustring(parsed.raw));

		impl->client = new GameClient();
		[s connectDisplaySignals];
		impl->client->change_name("Display");
		impl->client->change_color(0);
		impl->client->join_game("127.0.0.1", (unsigned int)parsed.port, true);

		impl->seats = parsed.seats;
		[s joinHostedSeatsFromIndex:0];

		[s rebuildAndNotify];
	});
	return YES;
}

// Returns YES once the seat at the given index has been assigned a player
// number by the server (remote seats are never joined and count as done).
- (BOOL)seatIsJoinedAtIndex:(int)index
{
	if (index < 0 || index >= (int)_impl->seats.size())
		return YES;

	Seat &seat = _impl->seats[index];
	if (seat.kind == CheechSeatRemote)
		return YES;

	GameClient *client = seat.client;
	if (seat.kind == CheechSeatComputer && seat.bot)
		client = seat.bot->get_game_client();
	if (!client)
		return NO;

	return client->get_my_player_number() != 0;
}

// Joins hosted seats one at a time, waiting for each to be assigned before
// joining the next, so that player numbers follow seat order.  Remote seats are
// skipped (another device fills them).  A short fallback timeout prevents a
// failed connection from stalling the rest of the seats indefinitely.
- (void)joinHostedSeatsFromIndex:(int)index
{
	SessionImpl *impl = _impl;

	while (index < (int)impl->seats.size()
		   && impl->seats[index].kind == CheechSeatRemote)
		index++;

	if (index >= (int)impl->seats.size())
	{
		[self rebuildAndNotify];
		return;
	}

	Seat &seat = impl->seats[index];
	unsigned int port = (unsigned int)impl->hostPort;

	if (seat.kind == CheechSeatHuman)
	{
		GameClient *client = new GameClient();
		[self connectSeatSignals:client index:index];
		if (!seat.name.empty())
			client->change_name(seat.name);
		client->change_color(seat.color);
		client->join_game("127.0.0.1", port, false);
		seat.client = client;
	}
	else if (seat.kind == CheechSeatComputer)
	{
		BotBase *bot = BotBase::new_bot_of_type(seat.botType);
		if (!bot) bot = BotBase::new_bot_of_type("LookAhead(3)");
		if (!bot) bot = BotBase::new_bot_of_type("Simple(1)");
		if (bot)
		{
			// Let the bot apply its move immediately; the UI replays it.
			bot->set_think_delay(0);
			bot->set_move_delay(0, 0);
			bot->set_smarts(seat.smarts);
			if (!seat.name.empty())
				bot->set_name(seat.name);
			bot->set_color(seat.color);
			bot->join_game("127.0.0.1", port);
			seat.bot = bot;
			impl->bots.push_back(bot);
		}
	}

	impl->joinWaitTicks = 0;
	impl->joinTimer.disconnect();

	__weak CheechSession *weakSelf = self;
	impl->joinTimer = Glib::signal_timeout().connect([weakSelf, index]() -> bool
	{
		CheechSession *s = weakSelf;
		if (!s) return false;
		SessionImpl *impl2 = s->_impl;

		if ([s seatIsJoinedAtIndex:index] || impl2->joinWaitTicks++ > 100)
		{
			[s joinHostedSeatsFromIndex:index + 1];
			return false;
		}
		return true;
	}, 10);
}

- (void)leave
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		[s stopCore];
		[s rebuildAndNotify];
	});
}

- (void)performAction:(void (^)(GameClient *client))block
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, block]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		GameClient *target = [s activeClient];

		// Host actions (change name/color) must come from a player, not the
		// spectator display client, and should still work while a computer or
		// remote seat is the active turn.  Fall back to any human seat, then to
		// a bot's player socket (a hosted game may have no local human seat at
		// all).
		if (!target && impl->isHost)
			for (Seat &seat : impl->seats)
				if (seat.client) { target = seat.client; break; }

		if (!target && impl->isHost)
			for (BotBase *bot : impl->bots)
			{
				GameClient *botClient = bot->get_game_client();
				if (botClient && botClient->ready()) { target = botClient; break; }
			}

		if (!target) target = impl->client;
		if (!target) return;
		block(target);
	});
}

// Host actions run against the in-process server directly, so they work even
// when this device has no local player seat (e.g. an all-computer or
// all-remote game).  The server broadcasts the resulting state to clients.
- (void)performServerAction:(void (^)(GameServer *server))block
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, block]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		if (!impl->server) return;
		block(impl->server);
	});
}

- (GameClient *)activeClient
{
	SessionImpl *impl = _impl;
	if (!impl) return nil;
	if (!impl->isHost) return impl->client;

	if (!impl->client) return nil;
	int current = (int)impl->client->get_current_player();
	auto it = impl->clientByNumber.find(current);
	if (it != impl->clientByNumber.end()) return it->second;
	return nil;
}

- (void)undoMove
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;

		// Undo is relative to the requesting player: the server rolls back to
		// just before their last move, taking any computer/remote replies with
		// it.  So send it from a local human seat when one exists, rather than
		// from whichever player (possibly a computer) happens to be to move.
		GameClient *target = nullptr;
		if (impl->isHost)
			for (Seat &seat : impl->seats)
				if (seat.client && seat.client->ready()) { target = seat.client; break; }
		if (!target) target = [s activeClient];
		if (!target) target = impl->client;
		if (!target) return;
		target->undo_move();
	});
}
- (void)restartGame { [self performServerAction:^(GameServer *server) { server->restart_game(); }]; }
- (void)rotatePlayers { [self performServerAction:^(GameServer *server) { server->rotate_players(); }]; }
- (void)shufflePlayers { [self performServerAction:^(GameServer *server) { server->shuffle_players(); }]; }

- (void)changeName:(NSString *)name
{
	NSString *value = [name copy];
	[self performAction:^(GameClient *c) { c->change_name([value UTF8String]); }];
}

- (void)changeColor:(NSInteger)color
{
	[self performAction:^(GameClient *c) { c->change_color((int)color); }];
}

- (void)setAnimationStepMs:(NSInteger)stepMs
{
	if (stepMs < 0) stepMs = 0;
	if (stepMs > kMaxAnimateStepMs) stepMs = kMaxAnimateStepMs;

	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, stepMs]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->animStepMs = (int)stepMs;
		impl->animDoneMs = (int)stepMs * 2;
	});
}

- (void)setComputerSmarts:(NSInteger)percent
{
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, percent]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->computerSmarts = (int)percent;
	});
}


#pragma mark - In-game setup

- (void)reconfigureGameNumPlayers:(NSInteger)numPlayers
						longJumps:(BOOL)longJumps
						hopOthers:(BOOL)hopOthers
					   stopOthers:(BOOL)stopOthers
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, numPlayers, longJumps, hopOthers, stopOthers]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;

		// The in-process server can be reconfigured directly; a joined game
		// must send GAME_RECONFIG from a player socket (spectators are
		// rejected, so fall back to a bot's player socket).
		if (impl->server)
		{
			impl->server->reconfigure_game((unsigned int)numPlayers, longJumps,
										   hopOthers, stopOthers);
			[s rebuildAndNotify];
			return;
		}

		GameClient *target = impl->client;
		if (target && target->is_spectator())
		{
			target = nullptr;
			for (BotBase *bot : impl->extraBots)
			{
				GameClient *botClient = bot->get_game_client();
				if (botClient && botClient->ready() && !botClient->is_spectator())
				{
					target = botClient;
					break;
				}
			}
		}
		if (!target || target->is_spectator()) return;

		target->reconfigure_game((unsigned int)numPlayers, longJumps, hopOthers, stopOthers);
		[s rebuildAndNotify];
	});
}

- (void)addComputerPlayerOfType:(NSString *)type
						   name:(NSString *)name
						  color:(NSInteger)color
{
	std::string typeStr = type ? [type UTF8String] : "";
	NSString *defaultName = [CheechSession defaultNameForComputerType:type];
	std::string nameStr = (name && name.length > 0)
						  ? std::string([name UTF8String])
						  : std::string([defaultName UTF8String]);

	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, typeStr, nameStr, color]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		if (!impl->client) return;

		// Bots connect to the same host as this client (loopback for a host,
		// the remote address for a joiner); the server fills the next open
		// player slot.
		std::string host = impl->client->get_host_name();
		unsigned int port = impl->client->get_port();
		if (host.empty() || port == 0) return;

		BotBase *bot = BotBase::new_bot_of_type(typeStr);
		if (!bot) bot = BotBase::new_bot_of_type("LookAhead(3)");
		if (!bot) return;

		bot->set_think_delay(0);
		bot->set_move_delay(0, 0);
		bot->set_smarts(impl->computerSmarts);
		if (!nameStr.empty()) bot->set_name(nameStr);
		if (color > 0) bot->set_color((int)color);
		bot->join_game(host, port);
		impl->extraBots.push_back(bot);
		impl->extraBotInfos.push_back({typeStr, nameStr, (int)color,
									   impl->computerSmarts});

		[s rebuildAndNotify];
	});
}

- (void)removeComputerPlayers
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		for (BotBase *bot : impl->extraBots)
		{
			bot->leave_game();
			delete bot;
		}
		impl->extraBots.clear();
		impl->extraBotInfos.clear();
		[s rebuildAndNotify];
	});
}


#pragma mark - Board interaction

- (void)tapHole:(NSInteger)hole
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf, hole]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		GameClient *active = [s activeClient];
		if (!active || !active->get_board()) return;
		if (active->is_spectator()) return;
		if (impl->status == CheechStatusWaiting) return;
		int my = (int)active->get_my_player_number();
		if (my == 0) return;

		// Only build a move when it is our turn (or the turn is not known yet);
		// otherwise the server would reject it and the selection would confuse.
		int current = (int)active->get_current_player();
		if (current != 0 && current != my) return;

		GameBoard *board = active->get_board();
		std::vector<unsigned int> &sel = impl->selection;
		auto it = std::find(sel.begin(), sel.end(), (unsigned int)hole);

		if (it == sel.end())
		{
			sel.push_back((unsigned int)hole);
			bool valid = false;
			if (sel.size() == 1)
			{
				GameHole *h = (*board)[(unsigned int)hole];
				valid = h && (int)h->get_current_player() == my;
			}
			else
			{
				valid = board->valid_move_list(sel, false);
			}

			if (!valid)
			{
				sel.pop_back();

				// Tapping a different one of our pegs starts a new move from
				// it, even when a path is already selected.
				GameHole *h = (*board)[(unsigned int)hole];
				if (h && (int)h->get_current_player() == my)
				{
					sel.clear();
					sel.push_back((unsigned int)hole);
					active->show_move(&sel);
				}
			}
			else
				active->show_move(&sel);
		}
		else
		{
			if (!sel.empty() && sel.back() == (unsigned int)hole)
				sel.pop_back();
			else
				while (!sel.empty() && sel.back() != (unsigned int)hole)
					sel.pop_back();

			if (sel.empty())
				active->hide_move();
			else
				active->show_move(&sel);
		}

		[s rebuildAndNotify];
	});
}

- (void)confirmMove
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		GameClient *active = [s activeClient];
		if (!active) return;

		if (impl->selection.size() > 1)
			active->make_move(&impl->selection);

		impl->selection.clear();
		[s rebuildAndNotify];
	});
}

- (void)clearSelection
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		GameClient *active = [s activeClient];
		if (!active) return;

		if (!impl->selection.empty())
		{
			impl->selection.clear();
			active->hide_move();
		}
		[s rebuildAndNotify];
	});
}

- (void)removeLastHop
{
	__weak CheechSession *weakSelf = self;
	cheech::Loop::instance().post([weakSelf]()
	{
		CheechSession *s = weakSelf;
		if (!s) return;
		SessionImpl *impl = s->_impl;
		GameClient *active = [s activeClient];
		if (!active) return;

		if (!impl->selection.empty())
		{
			impl->selection.pop_back();
			if (impl->selection.empty())
				active->hide_move();
			else
				active->show_move(&impl->selection);
		}
		[s rebuildAndNotify];
	});
}


#pragma mark - Signal wiring

// Per-human-seat signals.  The seat clients do not drive the shared board
// state (the spectator display client does); they only need to register their
// server-assigned player number and resolve name/color collisions.
- (void)connectSeatSignals:(GameClient *)client index:(int)index
{
	__weak CheechSession *weakSelf = self;

	client->cmd_set_player_number.connect([weakSelf, client, index](unsigned int n)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (index >= 0 && index < (int)impl->seats.size())
			impl->seats[index].playerNumber = (int)n;
		impl->clientByNumber[(int)n] = client;
		[s rebuildAndNotify];
	});

	client->cmd_choose_new_name.connect([weakSelf, client, index](Glib::ustring base)
	{
		CheechSession *s = weakSelf; if (!s) return;
		Glib::ustring newName = base + " " + std::to_string(index + 1);
		client->change_name(newName);
		[s rebuildAndNotify];
	});

	client->cmd_choose_new_color.connect([weakSelf, client](Glib::ustring, int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		GameBoard *board = client->get_board();
		int chosen = 1;
		if (board)
		{
			bool used[9] = {false};
			for (unsigned int p = 1; p <= 6; p++)
			{
				int c = (int)client->get_player_color(p);
				if (c >= 1 && c <= 8) used[c] = true;
			}
			for (int c = 1; c <= 8; c++)
				if (!used[c]) { chosen = c; break; }
		}
		client->change_color(chosen);
		[s rebuildAndNotify];
	});
}

- (void)connectDisplaySignals
{
	__weak CheechSession *weakSelf = self;
	GameClient *client = _impl->client;
	if (!client) return;

	client->evt_connected.connect([weakSelf]()
	{
		CheechSession *s = weakSelf; if (!s) return;
		s->_impl->status = CheechStatusWaiting;
		[s rebuildAndNotify];
	});

	client->evt_disconnected.connect([weakSelf]()
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->evt_cancelled.connect([weakSelf]()
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->cmd_set_player_number.connect([weakSelf](unsigned int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->cmd_player_add.connect([weakSelf](unsigned int, Glib::ustring, int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->cmd_player_remove.connect([weakSelf](unsigned int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->cmd_player_finish.connect([weakSelf](unsigned int posn, unsigned int move_count)
	{
		CheechSession *s = weakSelf; if (!s) return;
		if (posn >= 1 && posn <= 6) s->_impl->snap.finishedInMoves[posn] = (int)move_count;
		[s rebuildAndNotify];
	});

	client->cmd_game_resync.connect([weakSelf]()
	{
		CheechSession *s = weakSelf; if (!s) return;
		[s rebuildAndNotify];
	});

	client->cmd_game_turn.connect([weakSelf](unsigned int posn, GameServer::GameStatus status, unsigned int move_number)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		impl->status = (int)status;
		if (posn == 1 && move_number == 1)
			for (int i = 0; i < 7; i++)
			{
				impl->snap.finishedInMoves[i] = 0;
				impl->snap.movesTaken[i] = 0;
			}
		if (posn == 1 && move_number == 1)
			impl->snap.moveNumber = 0;
		[s rebuildAndNotify];
	});

	client->cmd_game_show_move.connect([weakSelf](MoveList *move)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (!move) { [s rebuildAndNotify]; return; }

		// This signal fires for the local player's own echoed selection and
		// for other players' previews.  Only the former belongs in the
		// selection; computer moves are animated from their make_move.
		int owner = 0;
		if (!move->empty() && impl->client)
		{
			GameBoard *board = impl->client->get_board();
			if (board && (*board)[(*move)[0]])
				owner = (int)(*board)[(*move)[0]]->get_current_player();
		}

		if (owner != 0 && owner == (int)impl->client->get_my_player_number())
		{
			impl->selection = *move;
			[s rebuildAndNotify];
		}
	});

	client->cmd_game_hide_move.connect([weakSelf]()
	{
		CheechSession *s = weakSelf; if (!s) return;
		s->_impl->selection.clear();
		[s rebuildAndNotify];
	});

	client->cmd_game_make_move.connect([weakSelf](MoveList *move)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (move && !move->empty())
		{
			impl->snap.lastMove.assign(move->begin(), move->end());
			impl->snap.moveSerial++;
			impl->snap.moveNumber++;
			impl->selection.clear();

			// The peg now sits on the final hole of the move, so its owner is
			// the player who just moved; count one move for them.
			if (impl->client)
			{
				GameBoard *board = impl->client->get_board();
				int mover = (board && (*board)[move->back()])
					? (int)(*board)[move->back()]->get_current_player() : 0;
				if (mover >= 1 && mover <= 6)
					impl->snap.movesTaken[mover]++;

				// This move will be replayed by the UI over the following
				// duration; hold the bots off until it finishes so two
				// computer moves never animate on top of each other.
				if (mover != (int)impl->client->get_my_player_number())
					cheech::extend_move_gate(impl->animStepMs * ((int)move->size() - 1)
											 + impl->animDoneMs);
			}
		}
		[s rebuildAndNotify];
	});

	client->cmd_game_undo_move.connect([weakSelf](unsigned int from, unsigned int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (impl->snap.moveNumber > 0) impl->snap.moveNumber--;
		if (impl->client)
		{
			GameBoard *board = impl->client->get_board();
			int mover = (board && (*board)[from])
				? (int)(*board)[from]->get_current_player() : 0;
			if (mover >= 1 && mover <= 6 && impl->snap.movesTaken[mover] > 0)
				impl->snap.movesTaken[mover]--;
		}
		[s rebuildAndNotify];
	});

	client->evt_message.connect([weakSelf](Glib::ustring message)
	{
		CheechSession *s = weakSelf; if (!s) return;
		NSString *text = ns(message);
		dispatch_async(dispatch_get_main_queue(), ^{
			CheechSession *s2 = weakSelf; if (!s2) return;
			if ([s2.delegate respondsToSelector:@selector(cheechSession:didReceiveMessage:)])
				[s2.delegate cheechSession:s2 didReceiveMessage:text];
		});
	});

	client->cmd_choose_new_name.connect([weakSelf](Glib::ustring name)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (!impl->client) return;
		impl->nameCounter++;
		Glib::ustring newName = name + " " + std::to_string(impl->nameCounter + 1);
		impl->client->change_name(newName);
		[s rebuildAndNotify];
	});

	client->cmd_choose_new_color.connect([weakSelf](Glib::ustring, int)
	{
		CheechSession *s = weakSelf; if (!s) return;
		SessionImpl *impl = s->_impl;
		if (!impl->client) return;
		GameBoard *board = impl->client->get_board();
		int chosen = 1;
		if (board)
		{
			bool used[9] = {false};
			for (unsigned int p = 1; p <= 6; p++)
			{
				int c = (int)impl->client->get_player_color(p);
				if (c >= 1 && c <= 8) used[c] = true;
			}
			for (int c = 1; c <= 8; c++)
				if (!used[c]) { chosen = c; break; }
		}
		impl->client->change_color(chosen);
		[s rebuildAndNotify];
	});
}


#pragma mark - Snapshot / teardown

- (void)stopCore
{
	SessionImpl *impl = _impl;
	if (!impl) return;

	impl->joinTimer.disconnect();

	// Hotseat seat clients (human seats) must be left before deletion, same
	// double-free hazard as the primary client.
	for (Seat &seat : impl->seats)
	{
		if (seat.client)
		{
			seat.client->leave_game();
			delete seat.client;
			seat.client = nullptr;
		}
	}
	impl->seats.clear();
	impl->clientByNumber.clear();

	for (BotBase *bot : impl->bots)
	{
		bot->leave_game();
		delete bot;
	}
	impl->bots.clear();

	// In-game computer players connect to this client's host; leave them too.
	for (BotBase *bot : impl->extraBots)
	{
		bot->leave_game();
		delete bot;
	}
	impl->extraBots.clear();
	impl->extraBotInfos.clear();

	// leave_game() closes the socket first so ~GameClient() does not
	// double-free the board (see GameClient::disconnected).
	if (impl->client)
	{
		impl->client->leave_game();
		delete impl->client;
		impl->client = nullptr;
	}

	if (impl->server)
	{
		delete impl->server;
		impl->server = nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->snap.moveNumber = 0;
		impl->snap.moveSerial = 0;
		impl->snap.lastMove.clear();
		for (int i = 0; i < 7; i++)
		{
			impl->snap.movesTaken[i] = 0;
			impl->snap.finishedInMoves[i] = 0;
		}
	}

	impl->selection.clear();
	impl->status = 0;
	impl->isHost = false;
	impl->isSpectator = false;
	impl->configuredPlayers = 0;
	impl->savedState.clear();
}

- (void)rebuildSnapshot
{
	SessionImpl *impl = _impl;
	if (!impl) return;

	std::lock_guard<std::mutex> lock(impl->mutex);
	Snapshot &snap = impl->snap;

	snap.connected = false;
	snap.isHost = impl->isHost;
	snap.isSpectator = impl->isSpectator;
	snap.activeSeatKind = CheechSeatUnassigned;
	snap.hasLocalHumanSeat = !impl->clientByNumber.empty();
	snap.hasRemoteSeat = false;
	for (const Seat &seat : impl->seats)
		if (seat.kind == CheechSeatRemote) { snap.hasRemoteSeat = true; break; }
	// The player number of the sole local human seat, or 0 when there is not
	// exactly one (no local humans, or a hotseat game with several).  Human
	// players connected from other devices (remote seats) do not count, so a
	// single local human still rotates even in a game with multiple humans.
	snap.localHumanPlayerNumber = (impl->clientByNumber.size() == 1)
		? impl->clientByNumber.begin()->first : 0;
	snap.status = impl->status;
	snap.currentPlayer = 0;
	snap.myPlayerNumber = 0;
	snap.numPlayers = impl->configuredPlayers;
	snap.serverHost.clear();
	snap.serverPort = 0;
	snap.longJumps = false;
	snap.hopOthers = true;
	snap.stopOthers = true;
	snap.extraBotCount = (int)impl->extraBots.size();
	snap.players.assign(7, PlayerInfo());
	snap.playerCount = 0;
	std::fill(std::begin(snap.holes), std::end(snap.holes), 0);
	std::fill(std::begin(snap.finished), std::end(snap.finished), false);
	std::fill(std::begin(snap.pegsInGoal), std::end(snap.pegsInGoal), 0);

	GameClient *client = impl->client;
	if (!client) { snap.selected.clear(); return; }

	snap.connected = client->ready();
	snap.myPlayerNumber = (int)client->get_my_player_number();
	snap.currentPlayer = (int)client->get_current_player();
	// In a hosted game the display client is internally a spectator, but from
	// the host's point of view they own the seats, so the UI must not treat
	// them as one.
	snap.isSpectator = impl->isHost ? false : client->is_spectator();
	snap.serverHost = client->get_host_name();
	snap.serverPort = (int)client->get_port();

	GameBoard *board = client->get_board();
	if (board)
	{
		snap.numPlayers = (int)board->get_num_players();
		snap.longJumps = board->get_long_jumps_allowed();
		snap.hopOthers = board->get_hop_others_allowed();
		snap.stopOthers = board->get_stop_others_allowed();
	}

	for (unsigned int p = 1; p <= 6; p++)
	{
		Glib::ustring name = client->get_player_name(p);
		if (!name.empty())
		{
			snap.players[p].name = name;
			snap.players[p].color = (int)client->get_player_color(p);
			snap.playerCount++;
		}
	}

	if (board)
	{
		for (unsigned int i = 0; i < GameBoard::SIZE; i++)
		{
			GameHole *h = (*board)[i];
			snap.holes[i] = h ? (int)h->get_current_player() : 0;
		}

		for (unsigned int p = 1; p <= 6; p++)
		{
			snap.finished[p] = board->player_finished(p);
			snap.pegsInGoal[p] = (int)board->get_num_pegs_in_goal(p);
		}
	}

	// Keep an opaque snapshot of a fully local game so the app can restore it
	// after being killed.  Networked games (remote seats, or a joined game) are
	// never saved, nor are games that have not started or have already ended.
	bool fullyLocal = impl->isHost && impl->server && board
		&& impl->configuredPlayers > 0;
	if (fullyLocal)
		for (const Seat &seat : impl->seats)
			if (seat.kind == CheechSeatRemote) { fullyLocal = false; break; }
	if (fullyLocal && (snap.playerCount < snap.numPlayers
					   || board->game_finished()))
		fullyLocal = false;
	impl->savedState = fullyLocal ? buildLocalSave(impl) : std::string();

	if (impl->isHost)
	{
		int current = (int)client->get_current_player();
		if (current >= 1)
		{
			if (impl->clientByNumber.find(current) != impl->clientByNumber.end())
				snap.activeSeatKind = CheechSeatHuman;
			else
			{
				bool isComputer = false;
				for (BotBase *bot : impl->bots)
				{
					GameClient *botClient = bot->get_game_client();
					if (botClient && (int)botClient->get_my_player_number() == current)
					{
						isComputer = true;
						break;
					}
				}
				if (isComputer)
					snap.activeSeatKind = CheechSeatComputer;
				else if (!client->get_player_name(current).empty())
					snap.activeSeatKind = CheechSeatRemote;
			}
		}
	}

	snap.selected.assign(impl->selection.begin(), impl->selection.end());
}

- (void)rebuildAndNotify
{
	[self rebuildSnapshot];
	__weak CheechSession *weakSelf = self;
	dispatch_async(dispatch_get_main_queue(), ^{
		CheechSession *s = weakSelf; if (!s) return;
		[s.delegate cheechSessionDidUpdate:s];
	});
}


#pragma mark - State accessors

- (NSString *)localGameSave
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (_impl->savedState.empty()) return nil;
	NSString *result = [NSString stringWithUTF8String:_impl->savedState.c_str()];
	return result;
}

- (BOOL)connected
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.connected;
}

- (BOOL)isHost
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.isHost;
}

- (BOOL)isSpectator
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.isSpectator;
}

- (CheechSeatKind)activeSeatKind
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (CheechSeatKind)_impl->snap.activeSeatKind;
}

- (BOOL)hasLocalHumanSeat
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.hasLocalHumanSeat;
}

- (BOOL)hasRemoteSeat
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.hasRemoteSeat;
}

- (NSInteger)localHumanPlayerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.localHumanPlayerNumber;
}

- (CheechStatus)status
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (CheechStatus)_impl->snap.status;
}

- (NSInteger)numPlayers
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.numPlayers;
}

- (NSInteger)myPlayerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.myPlayerNumber;
}

- (NSInteger)currentPlayer
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.currentPlayer;
}

- (NSInteger)moveNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.moveNumber;
}

- (NSInteger)playerCount
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.playerCount;
}

- (NSString *)serverHost
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return [NSString stringWithUTF8String:_impl->snap.serverHost.c_str()];
}

- (NSInteger)serverPort
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.serverPort;
}

- (BOOL)longJumps
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.longJumps;
}

- (BOOL)hopOthers
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.hopOthers;
}

- (BOOL)stopOthers
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.stopOthers;
}

- (NSInteger)extraComputerPlayerCount
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return _impl->snap.extraBotCount;
}

- (NSInteger)colorForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber >= (NSInteger)_impl->snap.players.size()) return 0;
	return _impl->snap.players[playerNumber].color;
}

- (NSString *)nameForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber >= (NSInteger)_impl->snap.players.size()) return @"";
	return ns(_impl->snap.players[playerNumber].name);
}

- (BOOL)finishedForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber > 6) return NO;
	return _impl->snap.finished[playerNumber];
}

- (NSInteger)finishedInMovesForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber > 6) return 0;
	return _impl->snap.finishedInMoves[playerNumber];
}

- (NSInteger)movesTakenForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber > 6) return 0;
	return _impl->snap.movesTaken[playerNumber];
}

- (NSInteger)pegsInGoalForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber > 6) return 0;
	return _impl->snap.pegsInGoal[playerNumber];
}

- (NSInteger)pegsForPlayer:(NSInteger)playerNumber
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (playerNumber < 1 || playerNumber > 6) return 0;
	NSInteger count = 0;
	for (unsigned int i = 0; i < GameBoard::SIZE; i++)
		if (_impl->snap.holes[i] == playerNumber) count++;
	return count;
}

- (NSInteger)playerAtHole:(NSInteger)hole
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	if (hole < 0 || hole >= (NSInteger)GameBoard::SIZE) return 0;
	return _impl->snap.holes[hole];
}

- (NSArray<NSNumber *> *)selectedHoles
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	NSMutableArray *array = [NSMutableArray array];
	for (int h : _impl->snap.selected)
		[array addObject:@(h)];
	return array;
}

- (NSArray<NSNumber *> *)lastMove
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	NSMutableArray *array = [NSMutableArray array];
	for (int h : _impl->snap.lastMove)
		[array addObject:@(h)];
	return array;
}

- (NSInteger)moveSerial
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (NSInteger)_impl->snap.moveSerial;
}

- (NSInteger)animationStepMs
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (NSInteger)_impl->animStepMs;
}

- (NSInteger)animationDoneMs
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (NSInteger)_impl->animDoneMs;
}

- (NSInteger)computerSmarts
{
	std::lock_guard<std::mutex> lock(_impl->mutex);
	return (NSInteger)_impl->computerSmarts;
}

@end
