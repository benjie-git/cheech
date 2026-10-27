/*
 *  Shared mobile session controller for cheech.
 *
 *  This is the platform-neutral core behind the iOS CheechSession bridge and
 *  the Android JNI bridge.  It owns the in-process server/client/bot objects
 *  and runs all of their work on cheech::Loop, so callers may invoke the
 *  methods from any thread.
 *
 *  Copyright (c) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef CHEECH_MOBILE_SESSION_CORE_HH
#define CHEECH_MOBILE_SESSION_CORE_HH

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "game_board.hh"

namespace cheech
{
namespace mobile
{

enum SeatKind
{
	SeatUnassigned = -1,
	SeatHuman = 0,
	SeatComputer = 1,
	SeatRemote = 2,
};

// One seat of a locally hosted game.  Friend/enemy lists are seat indices;
// nullopt means "everyone" and an empty vector means "nobody".
struct SeatSpec
{
	int kind = SeatHuman;
	std::string bot_type;
	std::string name;
	int color = 0;
	int smarts = 100;
	std::optional<std::vector<int>> friend_seats;
	std::optional<std::vector<int>> enemy_seats;
};

struct PlayerInfo
{
	std::string name;
	int color = 0;
};

struct Snapshot
{
	bool connected = false;
	bool is_host = false;
	bool is_spectator = false;
	int active_seat_kind = SeatUnassigned;
	bool has_local_human_seat = false;
	bool has_remote_seat = false;
	int local_human_player_number = 0;
	int status = 0;
	int num_players = 0;
	int my_player_number = 0;
	int current_player = 0;
	int player_count = 0;
	std::string server_host;
	int server_port = 0;
	bool long_jumps = true;
	bool hop_others = true;
	bool stop_others = true;
	int extra_computer_player_count = 0;
	int computer_player_count = 0;
	std::array<PlayerInfo, 7> players{};
	std::array<bool, 7> finished{};
	std::array<int, 7> finished_in_moves{};
	std::array<int, 7> moves_taken{};
	std::array<int, 7> pegs_in_goal{};
	std::array<int, GameBoard::SIZE> holes{};
	std::vector<int> selected;
	std::vector<int> telegraph;
	std::vector<int> last_move;
	unsigned int move_serial = 0;
	int move_number = 0;
	bool skip_replay = false;
	int animation_step_ms = 250;
	int animation_done_ms = 500;
	int computer_smarts = 100;
	int web_port = 0;
	bool web_ready = false;
};

class SessionCore
{
public:
	struct Impl;

	using UpdateHandler = std::function<void(const Snapshot &)>;
	using MessageHandler = std::function<void(const std::string &)>;

	SessionCore();
	~SessionCore();

	SessionCore(const SessionCore &) = delete;
	SessionCore &operator=(const SessionCore &) = delete;

	// Handlers are invoked from the cheech loop thread.  Platforms marshal
	// them to their UI thread themselves.
	void set_handlers(UpdateHandler update, MessageHandler message);

	// Directory containing the web client (index.html, css/, js/, ...).
	// Applied process-wide before the embedded web server serves a file.
	static void set_web_root(const std::string &path);

	void join_host(const std::string &host, unsigned int port, bool spectator,
		const std::string &player_name, int color);

	void start_game(unsigned int port, int num_players, bool long_jumps,
		bool hop_others, bool stop_others, const std::vector<SeatSpec> &seats,
		unsigned int web_port = 0);

	bool resume_local_game(const std::string &save, unsigned int web_port = 0);
	std::string local_game_save() const;

	void leave();

	void change_name(const std::string &name);
	void change_color(int color);
	void send_chat(const std::string &text);

	void set_animation_step_ms(int step_ms);
	void set_computer_smarts(int percent);

	void reconfigure_game(int num_players, bool long_jumps, bool hop_others,
		bool stop_others);
	void add_computer_player(const std::string &type, const std::string &name,
		int color, std::optional<std::vector<int>> friends,
		std::optional<std::vector<int>> enemies);
	void add_computer_player(const std::string &type, const std::string &name,
		int color);
	void remove_computer_players();

	void undo_move();
	void restart_game();
	void rotate_players();
	void shuffle_players();

	void tap_hole(int hole);
	void confirm_move();
	void clear_selection();
	void remove_last_hop();

	Snapshot snapshot() const;
	bool consume_replay_skip();
	void hold_computer_moves_for_animation_hops(int hops);

	// Stops the loop-owned core objects and waits for teardown to finish.
	// Called by the destructor; platforms may call it explicitly on shutdown.
	void shutdown();

	static int board_hole_count();
	static bool board_hole_present(int hole);
	static double board_hole_x(int hole);
	static double board_hole_y(int hole);
	static std::string default_name_for_computer_type(const std::string &type);
	static std::string type_name_for_computer_type(const std::string &type);

private:
	std::shared_ptr<Impl> impl_;
};

} // namespace mobile
} // namespace cheech

#endif
