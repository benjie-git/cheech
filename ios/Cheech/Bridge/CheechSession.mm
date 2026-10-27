//
//  CheechSession.mm
//  Cheech
//
//  Objective-C++ adapter around the shared mobile session core.
//
//  Copyright (c) 2005-2026 Ben Levitt
//
//  SPDX-License-Identifier: MIT
//

#import "CheechSession.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "mobile/cheech_session_core.hh"

namespace
{

using cheech::mobile::SeatSpec;
using cheech::mobile::SessionCore;
using cheech::mobile::Snapshot;

NSString *to_ns(const std::string &s)
{
	NSString *result = [NSString stringWithUTF8String:s.c_str()];
	return result ? result : @"";
}

std::string from_ns(NSString *s)
{
	return s ? std::string([s UTF8String] ?: "") : std::string();
}

std::optional<std::vector<int>> seat_indices(NSArray<NSNumber *> *numbers)
{
	if (!numbers)
		return std::nullopt;
	std::vector<int> result;
	result.reserve(numbers.count);
	for (NSNumber *number in numbers)
		result.push_back(number.intValue);
	return result;
}

} // namespace

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
{
	std::shared_ptr<SessionCore> _core;
}

- (instancetype)init
{
	self = [super init];
	if (self)
	{
		_core = std::make_shared<SessionCore>();

		__weak CheechSession *weakSelf = self;
		_core->set_handlers(
			[weakSelf](const Snapshot &snapshot)
			{
				Snapshot copy = snapshot;
				dispatch_async(dispatch_get_main_queue(), ^{
					CheechSession *session = weakSelf;
					if (!session)
						return;
					[session.delegate cheechSessionDidUpdate:session];
				});
			},
			[weakSelf](const std::string &message)
			{
				NSString *text = to_ns(message);
				dispatch_async(dispatch_get_main_queue(), ^{
					CheechSession *s = weakSelf;
					if (!s) return;
					if ([s.delegate respondsToSelector:@selector(cheechSession:didReceiveMessage:)])
						[s.delegate cheechSession:s didReceiveMessage:text];
				});
			});
	}
	return self;
}

- (void)dealloc
{
	_core->set_handlers(nullptr, nullptr);
	_core->shutdown();
	_core.reset();
}

#pragma mark - Static board geometry

+ (NSInteger)boardHoleCount
{
	return (NSInteger)SessionCore::board_hole_count();
}

+ (BOOL)boardHolePresent:(NSInteger)hole
{
	return SessionCore::board_hole_present((int)hole) ? YES : NO;
}

+ (double)boardHoleX:(NSInteger)hole
{
	return SessionCore::board_hole_x((int)hole);
}

+ (double)boardHoleY:(NSInteger)hole
{
	return SessionCore::board_hole_y((int)hole);
}

+ (NSString *)defaultNameForComputerType:(NSString *)type
{
	return to_ns(SessionCore::default_name_for_computer_type(from_ns(type)));
}

+ (NSString *)typeNameForComputerType:(NSString *)type
{
	return to_ns(SessionCore::type_name_for_computer_type(from_ns(type)));
}

#pragma mark - Setup

- (void)joinHost:(NSString *)host
			port:(uint16_t)port
	   spectator:(BOOL)spectator
	  playerName:(NSString *)playerName
		   color:(NSInteger)color
{
	_core->join_host(from_ns(host), port, spectator == YES,
		from_ns(playerName), (int)color);
}

- (void)startGameOnPort:(uint16_t)port
			 numPlayers:(NSInteger)numPlayers
			  longJumps:(BOOL)longJumps
			  hopOthers:(BOOL)hopOthers
			 stopOthers:(BOOL)stopOthers
				  seats:(NSArray<CheechSeat *> *)seats
{
	std::vector<SeatSpec> specs;
	specs.reserve(seats.count);
	for (CheechSeat *seat in seats)
	{
		SeatSpec spec;
		spec.kind = (int)seat.kind;
		spec.bot_type = from_ns(seat.botType);
		spec.name = from_ns(seat.name);
		spec.color = (int)seat.color;
		spec.smarts = (int)seat.smarts;
		spec.friend_seats = seat_indices(seat.friendSeats);
		spec.enemy_seats = seat_indices(seat.enemySeats);
		specs.push_back(spec);
	}

	_core->start_game(port, (int)numPlayers, longJumps == YES,
		hopOthers == YES, stopOthers == YES, specs);
}

- (nullable NSString *)localGameSave
{
	std::string save = _core->local_game_save();
	return save.empty() ? nil : to_ns(save);
}

- (BOOL)resumeLocalGame:(NSString *)save
{
	return _core->resume_local_game(from_ns(save)) ? YES : NO;
}

- (void)leave
{
	_core->leave();
}

- (void)changeName:(NSString *)name
{
	_core->change_name(from_ns(name));
}

- (void)changeColor:(NSInteger)color
{
	_core->change_color((int)color);
}

- (void)sendChat:(NSString *)text
{
	_core->send_chat(from_ns(text));
}

- (void)setAnimationStepMs:(NSInteger)stepMs
{
	_core->set_animation_step_ms((int)stepMs);
}

- (void)setComputerSmarts:(NSInteger)percent
{
	_core->set_computer_smarts((int)percent);
}

#pragma mark - In-game setup

- (void)reconfigureGameNumPlayers:(NSInteger)numPlayers
						longJumps:(BOOL)longJumps
						hopOthers:(BOOL)hopOthers
					   stopOthers:(BOOL)stopOthers
{
	_core->reconfigure_game((int)numPlayers, longJumps == YES,
		hopOthers == YES, stopOthers == YES);
}

- (void)addComputerPlayerOfType:(NSString *)type
						   name:(NSString *)name
						  color:(NSInteger)color
{
	_core->add_computer_player(from_ns(type), from_ns(name), (int)color);
}

- (void)addComputerPlayerOfType:(NSString *)type
						   name:(NSString *)name
						  color:(NSInteger)color
				   friendPlayers:(nullable NSArray<NSNumber *> *)friends
					enemyPlayers:(nullable NSArray<NSNumber *> *)enemies
{
	_core->add_computer_player(from_ns(type), from_ns(name), (int)color,
		seat_indices(friends), seat_indices(enemies));
}

- (void)removeComputerPlayers
{
	_core->remove_computer_players();
}

- (void)undoMove
{
	_core->undo_move();
}

- (void)restartGame
{
	_core->restart_game();
}

- (void)rotatePlayers
{
	_core->rotate_players();
}

- (void)shufflePlayers
{
	_core->shuffle_players();
}

#pragma mark - Board interaction

- (void)tapHole:(NSInteger)hole
{
	_core->tap_hole((int)hole);
}

- (void)confirmMove
{
	_core->confirm_move();
}

- (void)clearSelection
{
	_core->clear_selection();
}

- (void)removeLastHop
{
	_core->remove_last_hop();
}

#pragma mark - State accessors

- (BOOL)connected { return _core->snapshot().connected; }
- (BOOL)isHost { return _core->snapshot().is_host; }
- (BOOL)isSpectator { return _core->snapshot().is_spectator; }

- (CheechSeatKind)activeSeatKind
{
	return (CheechSeatKind)_core->snapshot().active_seat_kind;
}

- (BOOL)hasLocalHumanSeat { return _core->snapshot().has_local_human_seat; }
- (BOOL)hasRemoteSeat { return _core->snapshot().has_remote_seat; }

- (NSInteger)localHumanPlayerNumber
{
	return _core->snapshot().local_human_player_number;
}

- (CheechStatus)status { return (CheechStatus)_core->snapshot().status; }
- (NSInteger)numPlayers { return _core->snapshot().num_players; }
- (NSInteger)myPlayerNumber { return _core->snapshot().my_player_number; }
- (NSInteger)currentPlayer { return _core->snapshot().current_player; }
- (NSInteger)moveNumber { return _core->snapshot().move_number; }
- (NSInteger)playerCount { return _core->snapshot().player_count; }

- (NSString *)serverHost { return to_ns(_core->snapshot().server_host); }
- (NSInteger)serverPort { return _core->snapshot().server_port; }
- (BOOL)longJumps { return _core->snapshot().long_jumps; }
- (BOOL)hopOthers { return _core->snapshot().hop_others; }
- (BOOL)stopOthers { return _core->snapshot().stop_others; }

- (NSInteger)extraComputerPlayerCount
{
	return _core->snapshot().extra_computer_player_count;
}

- (NSInteger)computerPlayerCount
{
	return _core->snapshot().computer_player_count;
}

- (NSInteger)colorForPlayer:(NSInteger)playerNumber
{
	auto snapshot = _core->snapshot();
	if (playerNumber < 1 || playerNumber >= (NSInteger)snapshot.players.size())
		return 0;
	return snapshot.players[playerNumber].color;
}

- (NSString *)nameForPlayer:(NSInteger)playerNumber
{
	auto snapshot = _core->snapshot();
	if (playerNumber < 1 || playerNumber >= (NSInteger)snapshot.players.size())
		return @"";
	return to_ns(snapshot.players[playerNumber].name);
}

- (BOOL)finishedForPlayer:(NSInteger)playerNumber
{
	return (playerNumber >= 1 && playerNumber <= 6)
		? _core->snapshot().finished[playerNumber] : NO;
}

- (NSInteger)finishedInMovesForPlayer:(NSInteger)playerNumber
{
	return (playerNumber >= 1 && playerNumber <= 6)
		? _core->snapshot().finished_in_moves[playerNumber] : 0;
}

- (NSInteger)movesTakenForPlayer:(NSInteger)playerNumber
{
	return (playerNumber >= 1 && playerNumber <= 6)
		? _core->snapshot().moves_taken[playerNumber] : 0;
}

- (NSInteger)pegsInGoalForPlayer:(NSInteger)playerNumber
{
	return (playerNumber >= 1 && playerNumber <= 6)
		? _core->snapshot().pegs_in_goal[playerNumber] : 0;
}

- (NSInteger)pegsForPlayer:(NSInteger)playerNumber
{
	if (playerNumber < 1 || playerNumber > 6)
		return 0;
	auto snapshot = _core->snapshot();
	NSInteger count = 0;
	for (int owner : snapshot.holes)
		if (owner == playerNumber)
			count++;
	return count;
}

- (NSInteger)playerAtHole:(NSInteger)hole
{
	auto snapshot = _core->snapshot();
	if (hole < 0 || hole >= (NSInteger)snapshot.holes.size())
		return 0;
	return snapshot.holes[hole];
}

- (NSArray<NSNumber *> *)selectedHoles
{
	NSMutableArray *array = [NSMutableArray array];
	for (int hole : _core->snapshot().selected)
		[array addObject:@(hole)];
	return array;
}

- (NSArray<NSNumber *> *)telegraphHoles
{
	NSMutableArray *array = [NSMutableArray array];
	for (int hole : _core->snapshot().telegraph)
		[array addObject:@(hole)];
	return array;
}

- (BOOL)consumeReplaySkip
{
	return _core->consume_replay_skip() ? YES : NO;
}

- (NSArray<NSNumber *> *)lastMove
{
	NSMutableArray *array = [NSMutableArray array];
	for (int hole : _core->snapshot().last_move)
		[array addObject:@(hole)];
	return array;
}

- (NSInteger)moveSerial { return (NSInteger)_core->snapshot().move_serial; }
- (NSInteger)animationStepMs { return _core->snapshot().animation_step_ms; }
- (NSInteger)animationDoneMs { return _core->snapshot().animation_done_ms; }

- (void)holdComputerMovesForAnimationHops:(NSInteger)hops
{
	_core->hold_computer_moves_for_animation_hops((int)hops);
}

- (NSInteger)computerSmarts { return _core->snapshot().computer_smarts; }

@end
