//
//  CheechSession.h
//  Cheech
//
//  Objective-C bridge around the cheech C++ core.  All game logic runs on a
//  dedicated background thread (cheech::Loop); this object is safe to use from
//  the main thread and reports changes through CheechSessionDelegate.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, CheechStatus) {
	CheechStatusWaiting = 0,
	CheechStatusStart = 1,
	CheechStatusPlaying = 2,
	CheechStatusWon = 3,
	CheechStatusEnd = 4,
};

@class CheechSession;

typedef NS_ENUM(NSInteger, CheechSeatKind) {
	CheechSeatUnassigned = -1,
	CheechSeatHuman = 0,
	CheechSeatComputer = 1,
	CheechSeatRemote = 2,
};

// A single seat in a locally hosted game.  Human seats are played on this
// device, Computer seats are played by a bot, and Remote seats are left open
// for another device to fill by joining.
@interface CheechSeat : NSObject
@property (nonatomic) CheechSeatKind kind;
@property (nonatomic, copy) NSString *botType;
@property (nonatomic, copy) NSString *name;
@property (nonatomic) NSInteger color;
+ (instancetype)humanWithName:(NSString *)name color:(NSInteger)color;
+ (instancetype)computerWithType:(NSString *)type
							name:(NSString *)name
						   color:(NSInteger)color;
+ (instancetype)remoteSeat;
@end

@protocol CheechSessionDelegate <NSObject>
- (void)cheechSessionDidUpdate:(CheechSession *)session;
@optional
- (void)cheechSession:(CheechSession *)session didReceiveMessage:(NSString *)message;
@end

@interface CheechSession : NSObject

@property (nonatomic, weak, nullable) id<CheechSessionDelegate> delegate;

// Static board geometry (independent of any running game).
+ (NSInteger)boardHoleCount;
+ (BOOL)boardHolePresent:(NSInteger)hole;
+ (double)boardHoleX:(NSInteger)hole;
+ (double)boardHoleY:(NSInteger)hole;

// The fun default name for a bot type (e.g. "LookAhead(2)" -> "Earl"), or ""
// if the type is unknown.  Matches the old "Add Computer Player" flow.
+ (NSString *)defaultNameForComputerType:(NSString *)type;

// Connection / setup
- (void)joinHost:(NSString *)host
			port:(uint16_t)port
	   spectator:(BOOL)spectator
	  playerName:(NSString *)playerName
		   color:(NSInteger)color;

// Starts a locally hosted game.  An in-process server is created and each seat
// is assigned according to its kind: Human seats are played on this device,
// Computer seats by a bot, and Remote seats are left open for other devices to
// fill via joinHost:.  No networking setup is required on the host device.
- (void)startGameOnPort:(uint16_t)port
			 numPlayers:(NSInteger)numPlayers
			  longJumps:(BOOL)longJumps
			  hopOthers:(BOOL)hopOthers
			 stopOthers:(BOOL)stopOthers
				  seats:(NSArray<CheechSeat *> *)seats;

- (void)leave;

// Change this player's name/color while connected.
- (void)changeName:(NSString *)name;
- (void)changeColor:(NSInteger)color;

// Opponent move animation timing, in milliseconds per hop (0...500).  The
// resting time at the destination is always twice this value.
- (void)setAnimationStepMs:(NSInteger)stepMs;

// Host actions (ignored by the server for non-hosts)
- (void)undoMove;
- (void)restartGame;
- (void)rotatePlayers;
- (void)shufflePlayers;

// Board interaction
- (void)tapHole:(NSInteger)hole;
- (void)confirmMove;
- (void)clearSelection;

// State
@property (nonatomic, readonly) BOOL connected;
@property (nonatomic, readonly) BOOL isHost;
@property (nonatomic, readonly) BOOL isSpectator;
// The kind of seat whose turn it currently is (host games only); this is how
// the UI knows whether the local device may make the current move.
@property (nonatomic, readonly) CheechSeatKind activeSeatKind;
// Whether any seat is played locally by a human.
@property (nonatomic, readonly) BOOL hasLocalHumanSeat;
// The player number of the sole locally played human seat, or 0 if there is
// not exactly one local human (e.g. no humans, or a hotseat game with several).
@property (nonatomic, readonly) NSInteger localHumanPlayerNumber;
@property (nonatomic, readonly) CheechStatus status;
@property (nonatomic, readonly) NSInteger numPlayers;
@property (nonatomic, readonly) NSInteger myPlayerNumber;
@property (nonatomic, readonly) NSInteger currentPlayer;

// Global move number reported by the server (1-based); 0 before a game starts.
@property (nonatomic, readonly) NSInteger moveNumber;

@property (nonatomic, readonly) NSInteger playerCount;

// Address this client connected to: the server's host/port (a joiner's host is
// the address they joined; a host's is the loopback it serves on).  Used to
// invite more players once a game is joined.
@property (nonatomic, readonly) NSString *serverHost;
@property (nonatomic, readonly) NSInteger serverPort;
- (NSInteger)colorForPlayer:(NSInteger)playerNumber;
- (NSString *)nameForPlayer:(NSInteger)playerNumber;
- (BOOL)finishedForPlayer:(NSInteger)playerNumber;
- (NSInteger)finishedInMovesForPlayer:(NSInteger)playerNumber;
- (NSInteger)movesTakenForPlayer:(NSInteger)playerNumber;
- (NSInteger)pegsInGoalForPlayer:(NSInteger)playerNumber;
- (NSInteger)pegsForPlayer:(NSInteger)playerNumber;

- (NSInteger)playerAtHole:(NSInteger)hole;
- (NSArray<NSNumber *> *)selectedHoles;
- (NSArray<NSNumber *> *)lastMove;

// Bumped once for every move applied to the board.  The UI uses this to
// detect a fresh move and animate it; see MoveAnimator.
@property (nonatomic, readonly) NSInteger moveSerial;

// Suggested animation timing (milliseconds) for replaying a move: each hop
// takes animationStepMs and the peg rests at the destination for
// animationDoneMs before the animation is considered finished.
@property (nonatomic, readonly) NSInteger animationStepMs;
@property (nonatomic, readonly) NSInteger animationDoneMs;

@end

NS_ASSUME_NONNULL_END
