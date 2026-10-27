/*
 *  Shared mobile session controller for cheech.
 *
 *  Copyright (c) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "cheech_session_core.hh"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "cheech_loop.hh"
#include "game_server.hh"
#include "game_client.hh"
#include "game_board.hh"
#include "game_hole.hh"
#include "bot_base.hh"
#include "ajax_server.hh"
#include "cheech_move_gate.hh"
#include <glibmm/main.h>

namespace cheech
{
namespace mobile
{

namespace
{

std::string &web_root()
{
	static std::string root;
	return root;
}

void ensure_loop_started()
{
	static std::once_flag once;
	std::call_once(once, [] { cheech::Loop::instance().start(); });
}

// Client-side move animation timing defaults.  The bot itself moves
// immediately (a blocked event loop is undesirable on mobile), so the UI
// replays each hop over this duration instead of watching the bot step through
// the move.  The destination rest time is always twice the per-hop step.
const int k_animate_step_ms = 250;
const int k_max_animate_step_ms = 500;

} // namespace

struct SessionCore::Impl
{
	struct Seat
	{
		SeatKind kind = SeatHuman;
		std::string bot_type;
		std::string name;
		int color = 0;
		int smarts = 100;
		GameClient *client = nullptr; // when kind == SeatHuman
		BotBase *bot = nullptr;       // when kind == SeatComputer
		int player_number = 0;

		unsigned int player_id = 0;
		bool has_player_id = false;

		bool focus_configured = false;
		std::vector<int> friend_seats;
		std::vector<int> enemy_seats;
	};

	struct ExtraBotInfo
	{
		std::string type;
		std::string name;
		int color = 0;
		int smarts = 100;
		bool focus_configured = false;
		std::vector<std::string> friend_names;
		std::vector<std::string> enemy_names;
		std::vector<unsigned int> friend_ids;
		std::vector<unsigned int> enemy_ids;
	};

	GameServer *server = nullptr;
	GameClient *client = nullptr;
	std::vector<BotBase *> bots;
	std::vector<Seat> seats;
	std::unordered_map<int, GameClient *> client_by_number;
	std::vector<BotBase *> extra_bots;
	std::vector<ExtraBotInfo> extra_bot_infos;
	AjaxServer *web_server = nullptr;

	std::string saved_state;
	std::vector<unsigned int> selection;
	int configured_players = 0;
	int status = 0;
	bool is_host = false;
	bool is_spectator = false;

	Snapshot snap;
	mutable std::mutex mutex;
	int name_counter = 0;
	int anim_step_ms = k_animate_step_ms;
	int anim_done_ms = k_animate_step_ms * 2;
	int computer_smarts = 100;
	int host_port = 0;
	unsigned int web_port = 0;

	sigc::connection join_timer;
	int join_wait_ticks = 0;

	std::function<void(const Snapshot &)> update_handler;
	std::function<void(const std::string &)> message_handler;

	void stop_core();
	void rebuild_snapshot();
	void rebuild_and_notify();
	void connect_display_signals();
	void connect_seat_signals(GameClient *client, int index);
	void join_hosted_seats_from_index(int index);
	bool seat_is_joined_at_index(int index);
	GameClient *active_client();
	void perform_action(const std::function<void(GameClient *)> &block);
	void perform_server_action(const std::function<void(GameServer *)> &block);
	void start_web_server(unsigned int port);
	void stop_web_server();
	void notify_update();
	void notify_message(const std::string &message);
};

namespace
{

using Seat = SessionCore::Impl::Seat;
using ExtraBotInfo = SessionCore::Impl::ExtraBotInfo;

int player_number_for_seat(SessionCore::Impl *impl, int seat_index)
{
	if (!impl || seat_index < 0 || seat_index >= (int)impl->seats.size())
		return 0;

	Seat &seat = impl->seats[seat_index];
	if (seat.player_number > 0)
		return seat.player_number;
	if (seat.kind == SeatComputer && seat.bot)
	{
		GameClient *client = seat.bot->get_game_client();
		if (client)
			return (int)client->get_my_player_number();
	}

	return 0;
}

unsigned int id_for_seat(SessionCore::Impl *impl, int seat_index)
{
	if (!impl || seat_index < 0 || seat_index >= (int)impl->seats.size())
		return 0;

	Seat &seat = impl->seats[seat_index];
	if (seat.has_player_id && seat.player_id)
		return seat.player_id;

	GameClient *client = seat.client;
	if (seat.kind == SeatComputer && seat.bot)
		client = seat.bot->get_game_client();

	if (client)
	{
		unsigned int id = client->get_my_player_id();
		if (id)
		{
			seat.player_id = id;
			seat.has_player_id = true;
			return id;
		}
	}

	if (!seat.has_player_id && impl->client)
	{
		int posn = player_number_for_seat(impl, seat_index);
		if (posn > 0)
		{
			unsigned int id = impl->client->get_player_id((unsigned int)posn);
			if (id)
			{
				seat.player_id = id;
				seat.has_player_id = true;
				return id;
			}
		}
	}

	return seat.has_player_id ? seat.player_id : 0;
}

std::set<unsigned int> id_set_for_seat_indices(SessionCore::Impl *impl,
	const std::vector<int> &indices)
{
	std::set<unsigned int> ids;
	for (int index : indices)
	{
		unsigned int id = id_for_seat(impl, index);
		if (id)
			ids.insert(id);
	}
	return ids;
}

void associate_remote_seats(SessionCore::Impl *impl)
{
	if (!impl || !impl->client || !impl->client->get_board())
		return;

	unsigned int num_players = impl->client->get_board()->get_num_players();

	for (unsigned int posn = 1; posn <= num_players; posn++)
	{
		if (impl->client->get_player_name(posn).empty())
			continue;

		bool claimed = false;
		for (int i = 0; i < (int)impl->seats.size() && !claimed; i++)
		{
			Seat &seat = impl->seats[i];
			if (seat.kind == SeatRemote)
				claimed = (seat.player_number == (int)posn);
			else
				claimed = (player_number_for_seat(impl, i) == (int)posn);
		}
		if (claimed)
			continue;

		for (BotBase *bot : impl->extra_bots)
		{
			if (bot && bot->get_game_client()->get_my_player_number() == posn)
			{
				claimed = true;
				break;
			}
		}
		if (claimed)
			continue;

		for (Seat &seat : impl->seats)
		{
			if (seat.kind == SeatRemote && seat.player_number == 0)
			{
				seat.player_number = (int)posn;
				break;
			}
		}
	}
}

void apply_seat_focus(SessionCore::Impl *impl)
{
	if (!impl)
		return;

	for (Seat &seat : impl->seats)
	{
		if (seat.kind != SeatComputer || !seat.bot || !seat.focus_configured)
			continue;

		std::set<unsigned int> friends =
			id_set_for_seat_indices(impl, seat.friend_seats);
		std::set<unsigned int> enemies =
			id_set_for_seat_indices(impl, seat.enemy_seats);

		if (friends.size() == seat.friend_seats.size())
			seat.bot->set_friend_ids(friends);
		if (enemies.size() == seat.enemy_seats.size())
			seat.bot->set_enemy_ids(enemies);
	}
}

unsigned int mask_for_names(SessionCore::Impl *impl,
	const std::vector<std::string> &names)
{
	unsigned int mask = 0;
	if (!impl->client || !impl->client->get_board())
		return mask;

	unsigned int num_players = impl->client->get_board()->get_num_players();
	for (unsigned int player = 1; player <= num_players && player <= 31; player++)
	{
		std::string name = impl->client->get_player_name(player);
		if (std::find(names.begin(), names.end(), name) != names.end())
			mask |= (1u << (player - 1));
	}
	return mask;
}

unsigned int id_for_player_name(SessionCore::Impl *impl, const std::string &name)
{
	if (!impl->client || !impl->client->get_board() || name.empty())
		return 0;

	unsigned int num_players = impl->client->get_board()->get_num_players();
	for (unsigned int posn = 1; posn <= num_players; posn++)
	{
		if (std::string(impl->client->get_player_name(posn)) == name)
			return impl->client->get_player_id(posn);
	}
	return 0;
}

void apply_extra_bot_focus(SessionCore::Impl *impl)
{
	if (!impl)
		return;

	for (size_t i = 0; i < impl->extra_bots.size() && i < impl->extra_bot_infos.size(); i++)
	{
		ExtraBotInfo &info = impl->extra_bot_infos[i];
		if (!info.focus_configured || !impl->extra_bots[i])
			continue;

		if (info.friend_ids.empty())
			for (const std::string &name : info.friend_names)
			{
				unsigned int id = id_for_player_name(impl, name);
				if (id)
					info.friend_ids.push_back(id);
			}
		if (info.enemy_ids.empty())
			for (const std::string &name : info.enemy_names)
			{
				unsigned int id = id_for_player_name(impl, name);
				if (id)
					info.enemy_ids.push_back(id);
			}

		if (!info.friend_ids.empty())
		{
			std::set<unsigned int> ids(info.friend_ids.begin(), info.friend_ids.end());
			impl->extra_bots[i]->set_friend_ids(ids);
		}
		else
		{
			impl->extra_bots[i]->set_friends(mask_for_names(impl, info.friend_names));
		}

		if (!info.enemy_ids.empty())
		{
			std::set<unsigned int> ids(info.enemy_ids.begin(), info.enemy_ids.end());
			impl->extra_bots[i]->set_enemy_ids(ids);
		}
		else
		{
			impl->extra_bots[i]->set_enemies(mask_for_names(impl, info.enemy_names));
		}
	}
}

std::string build_local_save(SessionCore::Impl *impl)
{
	std::ostringstream out;
	out << "CHEECHSAVE 1\n";
	out << "port " << impl->host_port << "\n";
	out << "seats " << (impl->seats.size() + impl->extra_bot_infos.size()) << "\n";

	std::vector<std::string> names;
	for (const Seat &seat : impl->seats)
		names.push_back(seat.name);
	for (const ExtraBotInfo &bot : impl->extra_bot_infos)
		names.push_back(bot.name);

	auto name_index = [&names](const std::string &name) -> int
	{
		if (name.empty())
			return -1;
		for (size_t i = 0; i < names.size(); i++)
			if (names[i] == name)
				return (int)i;
		return -1;
	};
	auto csv = [](const std::vector<int> &indices) -> std::string
	{
		std::ostringstream s;
		for (size_t i = 0; i < indices.size(); i++)
		{
			if (i)
				s << ",";
			s << indices[i];
		}
		return s.str();
	};

	int index = 0;
	for (const Seat &seat : impl->seats)
	{
		out << "seat " << (int)seat.kind << " " << seat.color << " "
			<< seat.smarts << " "
			<< (seat.bot_type.empty() ? std::string("-") : seat.bot_type) << " "
			<< seat.name << "\n";
		if (seat.focus_configured)
			out << "seatfocus " << index << " "
				<< csv(seat.friend_seats) << " " << csv(seat.enemy_seats) << "\n";
		index++;
	}
	for (const ExtraBotInfo &bot : impl->extra_bot_infos)
	{
		out << "seat " << (int)SeatComputer << " " << bot.color << " "
			<< bot.smarts << " "
			<< (bot.type.empty() ? std::string("-") : bot.type) << " "
			<< bot.name << "\n";
		if (bot.focus_configured)
		{
			std::vector<int> friends, enemies;
			for (const std::string &name : bot.friend_names)
			{
				int idx = name_index(name);
				if (idx >= 0)
					friends.push_back(idx);
			}
			for (const std::string &name : bot.enemy_names)
			{
				int idx = name_index(name);
				if (idx >= 0)
					enemies.push_back(idx);
			}
			out << "seatfocus " << index << " "
				<< csv(friends) << " " << csv(enemies) << "\n";
		}
		index++;
	}
	out << std::string(impl->server->save_state().c_str());
	return out.str();
}

struct LocalSave
{
	int port = 0;
	int num_players = 0;
	bool long_jumps = true;
	bool hop_others = true;
	bool stop_others = true;
	std::vector<Seat> seats;
	std::string raw;
	bool valid = false;
};

LocalSave parse_local_save(const std::string &text)
{
	LocalSave save;
	save.raw = text;
	std::map<int, std::pair<std::vector<int>, std::vector<int>>> focus;
	auto parse_csv = [](const std::string &csv) -> std::vector<int>
	{
		std::vector<int> out;
		std::istringstream in(csv);
		std::string item;
		while (std::getline(in, item, ','))
		{
			if (!item.empty())
				out.push_back(std::atoi(item.c_str()));
		}
		return out;
	};
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
			in >> save.num_players;
		else if (key == "rules")
		{
			int lj = 0, ho = 1, so = 1;
			in >> lj >> ho >> so;
			save.long_jumps = lj != 0;
			save.hop_others = ho != 0;
			save.stop_others = so != 0;
		}
		else if (key == "seat")
		{
			Seat seat;
			int kind = SeatHuman, color = 0, smarts = 100;
			std::string bot_type;
			in >> kind >> color >> smarts >> bot_type;
			std::string name;
			std::getline(in, name);
			if (!name.empty() && name[0] == ' ')
				name.erase(0, 1);
			seat.kind = (SeatKind)kind;
			seat.color = color;
			seat.smarts = smarts;
			seat.bot_type = (bot_type == "-") ? "" : bot_type;
			seat.name = name;
			save.seats.push_back(seat);
		}
		else if (key == "seatfocus")
		{
			int index = -1;
			std::string friends, enemies;
			in >> index >> friends >> enemies;
			if (index >= 0)
				focus[index] = std::make_pair(parse_csv(friends), parse_csv(enemies));
		}
	}
	for (auto &entry : focus)
	{
		if (entry.first >= 0 && entry.first < (int)save.seats.size())
		{
			save.seats[entry.first].focus_configured = true;
			save.seats[entry.first].friend_seats = entry.second.first;
			save.seats[entry.first].enemy_seats = entry.second.second;
		}
	}
	save.valid = save.port > 0 && save.port <= 65535
		&& save.num_players >= 2 && save.num_players <= 6
		&& !save.seats.empty() && save.seats.size() <= 6;
	return save;
}

} // namespace

void SessionCore::Impl::notify_update()
{
	Snapshot copy;
	{
		std::lock_guard<std::mutex> lock(mutex);
		copy = snap;
	}
	if (update_handler)
		update_handler(copy);
}

void SessionCore::Impl::notify_message(const std::string &message)
{
	if (message_handler)
		message_handler(message);
}

void SessionCore::Impl::start_web_server(unsigned int port)
{
	stop_web_server();
	web_port = port;
	if (port == 0 || !server)
		return;

	web_server = new AjaxServer();
	if (!web_server->listen(port, "127.0.0.1", (unsigned int)host_port))
	{
		delete web_server;
		web_server = nullptr;
		web_port = 0;
	}
}

void SessionCore::Impl::stop_web_server()
{
	if (web_server)
	{
		web_server->close();
		delete web_server;
		web_server = nullptr;
	}
	web_port = 0;
}

void SessionCore::Impl::stop_core()
{
	join_timer.disconnect();
	stop_web_server();

	for (Seat &seat : seats)
	{
		if (seat.client)
		{
			seat.client->leave_game();
			delete seat.client;
			seat.client = nullptr;
		}
	}
	seats.clear();
	client_by_number.clear();

	for (BotBase *bot : bots)
	{
		bot->leave_game();
		delete bot;
	}
	bots.clear();

	for (BotBase *bot : extra_bots)
	{
		bot->leave_game();
		delete bot;
	}
	extra_bots.clear();
	extra_bot_infos.clear();

	if (client)
	{
		client->leave_game();
		delete client;
		client = nullptr;
	}

	if (server)
	{
		delete server;
		server = nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(mutex);
		snap.move_number = 0;
		snap.move_serial = 0;
		snap.skip_replay = false;
		snap.last_move.clear();
		snap.telegraph.clear();
		for (int i = 0; i < 7; i++)
		{
			snap.moves_taken[i] = 0;
			snap.finished_in_moves[i] = 0;
		}
	}

	selection.clear();
	status = 0;
	is_host = false;
	is_spectator = false;
	configured_players = 0;
	saved_state.clear();
	web_port = 0;
}

bool SessionCore::Impl::seat_is_joined_at_index(int index)
{
	if (index < 0 || index >= (int)seats.size())
		return true;

	Seat &seat = seats[index];
	if (seat.kind == SeatRemote)
		return true;

	GameClient *seat_client = seat.client;
	if (seat.kind == SeatComputer && seat.bot)
		seat_client = seat.bot->get_game_client();
	if (!seat_client)
		return false;

	return seat_client->get_my_player_number() != 0;
}

void SessionCore::Impl::join_hosted_seats_from_index(int index)
{
	while (index < (int)seats.size() && seats[index].kind == SeatRemote)
		index++;

	if (index >= (int)seats.size())
	{
		apply_seat_focus(this);
		rebuild_and_notify();
		return;
	}

	Seat &seat = seats[index];
	unsigned int port = (unsigned int)host_port;

	if (seat.kind == SeatHuman)
	{
		GameClient *seat_client = new GameClient();
		connect_seat_signals(seat_client, index);
		if (!seat.name.empty())
			seat_client->change_name(seat.name);
		seat_client->change_color(seat.color);
		seat_client->join_game("127.0.0.1", port, false);
		seat.client = seat_client;
	}
	else if (seat.kind == SeatComputer)
	{
		BotBase *bot = BotBase::new_bot_of_type(seat.bot_type);
		if (!bot)
			bot = BotBase::new_bot_of_type("LookAhead(3)");
		if (!bot)
			bot = BotBase::new_bot_of_type("Simple(1)");
		if (bot)
		{
			bot->set_think_delay(0);
			bot->set_move_delay(0);
			bot->set_smarts(seat.smarts);
			if (!seat.name.empty())
				bot->set_name(seat.name);
			bot->set_color(seat.color);
			bot->join_game("127.0.0.1", port);
			seat.bot = bot;
			bots.push_back(bot);
		}
	}

	join_wait_ticks = 0;
	join_timer.disconnect();

	join_timer = Glib::signal_timeout().connect([this, index]() -> bool
	{
		if (seat_is_joined_at_index(index) || join_wait_ticks++ > 100)
		{
			apply_seat_focus(this);
			join_hosted_seats_from_index(index + 1);
			return false;
		}
		return true;
	}, 10);
}

GameClient *SessionCore::Impl::active_client()
{
	if (!is_host)
		return client;

	if (!client)
		return nullptr;
	int current = (int)client->get_current_player();
	auto it = client_by_number.find(current);
	if (it != client_by_number.end())
		return it->second;
	return nullptr;
}

void SessionCore::Impl::perform_action(const std::function<void(GameClient *)> &block)
{
	GameClient *target = active_client();

	if (!target && is_host)
		for (Seat &seat : seats)
			if (seat.client)
			{
				target = seat.client;
				break;
			}

	if (!target && is_host)
		for (BotBase *bot : bots)
		{
			GameClient *bot_client = bot->get_game_client();
			if (bot_client && bot_client->ready())
			{
				target = bot_client;
				break;
			}
		}

	if (!target)
		target = client;
	if (!target)
		return;
	block(target);
}

void SessionCore::Impl::perform_server_action(const std::function<void(GameServer *)> &block)
{
	if (!server)
		return;
	block(server);
}

int SessionCore::board_hole_count()
{
	return GameBoard::SIZE;
}

bool SessionCore::board_hole_present(int hole)
{
	if (hole < 0 || hole >= GameBoard::SIZE)
		return false;
	return GameBoard::BOARD_MAP[hole] >= 0;
}

double SessionCore::board_hole_x(int hole)
{
	int col = hole % GameBoard::SIZE_X;
	int row = hole / GameBoard::SIZE_X;
	return (double)col + ((row % 2 == 0) ? 0.5 : 0.0);
}

double SessionCore::board_hole_y(int hole)
{
	int row = hole / GameBoard::SIZE_X;
	return (double)row * (sqrt(3.0) / 2.0);
}

std::string SessionCore::default_name_for_computer_type(const std::string &type)
{
	if (type.empty())
		return "";

	BotBase *bot = BotBase::new_bot_of_type(type);
	if (!bot)
		return "";

	Glib::ustring name = bot->get_default_name();
	delete bot;
	return name;
}

std::string SessionCore::type_name_for_computer_type(const std::string &type)
{
	if (type.empty())
		return "";

	BotBase *bot = BotBase::new_bot_of_type(type);
	if (!bot)
		return "";

	Glib::ustring name = bot->get_type_name();
	delete bot;
	return name;
}

SessionCore::SessionCore()
	: impl_(std::make_shared<Impl>())
{
	ensure_loop_started();
}

SessionCore::~SessionCore()
{
	auto impl = impl_;
	impl_.reset();
	if (!impl)
		return;
	if (cheech::Loop::instance().running())
		cheech::Loop::instance().post([impl]() { impl->stop_core(); });
	else
		impl->stop_core();
}

void SessionCore::set_handlers(UpdateHandler update, MessageHandler message)
{
	impl_->update_handler = std::move(update);
	impl_->message_handler = std::move(message);
}

void SessionCore::set_web_root(const std::string &path)
{
	web_root() = path;
	if (!path.empty())
		setenv("CHEECH_WEB_ROOT", path.c_str(), 1);
	else
		unsetenv("CHEECH_WEB_ROOT");
}

void SessionCore::join_host(const std::string &host, unsigned int port,
	bool spectator, const std::string &player_name, int color)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, host, port, spectator, player_name, color]()
	{
		impl->stop_core();
		impl->is_host = false;
		impl->is_spectator = spectator;
		impl->configured_players = 0;
		impl->status = 0;

		impl->client = new GameClient();
		impl->connect_display_signals();
		impl->client->change_name(player_name);
		impl->client->change_color(color);
		impl->client->join_game(host, port, spectator);

		impl->rebuild_and_notify();
	});
}

void SessionCore::start_game(unsigned int port, int num_players,
	bool long_jumps, bool hop_others, bool stop_others,
	const std::vector<SeatSpec> &seats, unsigned int web_port)
{
	std::vector<Seat> specs;
	specs.reserve(seats.size());
	for (const SeatSpec &spec : seats)
	{
		Seat seat;
		seat.kind = (SeatKind)spec.kind;
		seat.bot_type = spec.bot_type;
		seat.name = spec.name;
		seat.color = spec.color;
		seat.smarts = spec.smarts;
		if (spec.friend_seats || spec.enemy_seats)
		{
			seat.focus_configured = true;
			if (spec.friend_seats)
				seat.friend_seats = *spec.friend_seats;
			if (spec.enemy_seats)
				seat.enemy_seats = *spec.enemy_seats;
		}
		specs.push_back(seat);
	}

	auto impl = impl_;
	cheech::Loop::instance().post([impl, port, num_players, long_jumps,
		hop_others, stop_others, specs, web_port]()
	{
		impl->stop_core();
		impl->is_host = true;
		impl->is_spectator = false;
		impl->configured_players = num_players;
		impl->status = 0;
		impl->host_port = (int)port;

		impl->server = new GameServer(port, (unsigned int)num_players,
			long_jumps, hop_others, stop_others);
		impl->server->new_game();

		impl->client = new GameClient();
		impl->connect_display_signals();
		impl->client->change_name("Display");
		impl->client->change_color(0);
		impl->client->join_game("127.0.0.1", port, true);

		impl->seats = specs;
		impl->join_hosted_seats_from_index(0);
		impl->start_web_server(web_port);
		impl->rebuild_and_notify();
	});
}

bool SessionCore::resume_local_game(const std::string &save, unsigned int web_port)
{
	if (save.empty())
		return false;
	LocalSave parsed = parse_local_save(save);
	if (!parsed.valid)
		return false;

	auto impl = impl_;
	cheech::Loop::instance().post([impl, parsed, web_port]()
	{
		impl->stop_core();
		impl->is_host = true;
		impl->is_spectator = false;
		impl->configured_players = parsed.num_players;
		impl->status = 0;
		impl->host_port = parsed.port;

		impl->server = new GameServer((unsigned int)parsed.port,
			(unsigned int)parsed.num_players, parsed.long_jumps,
			parsed.hop_others, parsed.stop_others);
		impl->server->new_game();
		impl->server->load_state(Glib::ustring(parsed.raw));

		impl->client = new GameClient();
		impl->connect_display_signals();
		impl->client->change_name("Display");
		impl->client->change_color(0);
		impl->client->join_game("127.0.0.1", (unsigned int)parsed.port, true);

		impl->seats = parsed.seats;
		impl->join_hosted_seats_from_index(0);
		impl->start_web_server(web_port);
		impl->rebuild_and_notify();
	});
	return true;
}

void SessionCore::leave()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		impl->stop_core();
		impl->rebuild_and_notify();
	});
}

std::string SessionCore::local_game_save() const
{
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->saved_state;
}

void SessionCore::change_name(const std::string &name)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, name]()
	{
		impl->perform_action([&name](GameClient *client) { client->change_name(name); });
	});
}

void SessionCore::change_color(int color)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, color]()
	{
		impl->perform_action([color](GameClient *client) { client->change_color(color); });
	});
}

void SessionCore::send_chat(const std::string &text)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, text]()
	{
		impl->perform_action([&text](GameClient *client) { client->chat(text); });
	});
}

void SessionCore::set_animation_step_ms(int step_ms)
{
	step_ms = std::max(0, std::min(step_ms, k_max_animate_step_ms));
	auto impl = impl_;
	cheech::Loop::instance().post([impl, step_ms]()
	{
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->anim_step_ms = step_ms;
		impl->anim_done_ms = step_ms * 2;
	});
}

void SessionCore::set_computer_smarts(int percent)
{
	percent = std::max(0, std::min(percent, 100));
	auto impl = impl_;
	cheech::Loop::instance().post([impl, percent]()
	{
		std::lock_guard<std::mutex> lock(impl->mutex);
		impl->computer_smarts = percent;
	});
}

void SessionCore::reconfigure_game(int num_players, bool long_jumps,
	bool hop_others, bool stop_others)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, num_players, long_jumps, hop_others, stop_others]()
	{
		if (impl->server)
		{
			impl->server->reconfigure_game((unsigned int)num_players,
				long_jumps, hop_others, stop_others);
			impl->rebuild_and_notify();
			return;
		}

		GameClient *target = impl->client;
		if (target && target->is_spectator())
		{
			target = nullptr;
			for (BotBase *bot : impl->extra_bots)
			{
				GameClient *bot_client = bot->get_game_client();
				if (bot_client && bot_client->ready() && !bot_client->is_spectator())
				{
					target = bot_client;
					break;
				}
			}
		}
		if (!target || target->is_spectator())
			return;

		target->reconfigure_game((unsigned int)num_players, long_jumps,
			hop_others, stop_others);
		impl->rebuild_and_notify();
	});
}

void SessionCore::add_computer_player(const std::string &type,
	const std::string &name, int color,
	std::optional<std::vector<int>> friends,
	std::optional<std::vector<int>> enemies)
{
	const std::string type_str = type;
	const std::string name_str = name.empty()
		? default_name_for_computer_type(type) : name;
	const std::vector<int> friend_nums = friends.value_or(std::vector<int>());
	const std::vector<int> enemy_nums = enemies.value_or(std::vector<int>());
	const bool focus_configured = friends.has_value() || enemies.has_value();

	auto impl = impl_;
	cheech::Loop::instance().post([impl, type_str, name_str, color,
		friend_nums, enemy_nums, focus_configured]()
	{
		if (!impl->client)
			return;

		std::string host = impl->client->get_host_name();
		unsigned int port = impl->client->get_port();
		if (host.empty() || port == 0)
			return;

		BotBase *bot = BotBase::new_bot_of_type(type_str);
		if (!bot)
			bot = BotBase::new_bot_of_type("LookAhead(3)");
		if (!bot)
			return;

		bot->set_think_delay(0);
		bot->set_move_delay(0);
		bot->set_smarts(impl->computer_smarts);
		if (!name_str.empty())
			bot->set_name(name_str);
		if (color > 0)
			bot->set_color(color);
		bot->join_game(host, port);
		impl->extra_bots.push_back(bot);

		ExtraBotInfo info;
		info.type = type_str;
		info.name = name_str;
		info.color = color;
		info.smarts = impl->computer_smarts;
		if (focus_configured)
		{
			info.focus_configured = true;
			for (int player : friend_nums)
				if (player > 0)
				{
					info.friend_names.push_back(std::string(
						impl->client->get_player_name((unsigned int)player)));
					unsigned int id = impl->client->get_player_id((unsigned int)player);
					if (id)
						info.friend_ids.push_back(id);
				}
			for (int player : enemy_nums)
				if (player > 0)
				{
					info.enemy_names.push_back(std::string(
						impl->client->get_player_name((unsigned int)player)));
					unsigned int id = impl->client->get_player_id((unsigned int)player);
					if (id)
						info.enemy_ids.push_back(id);
				}
		}
		impl->extra_bot_infos.push_back(info);

		apply_extra_bot_focus(impl.get());
		impl->rebuild_and_notify();
	});
}

void SessionCore::add_computer_player(const std::string &type,
	const std::string &name, int color)
{
	add_computer_player(type, name, color, std::nullopt, std::nullopt);
}

void SessionCore::remove_computer_players()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		for (Seat &seat : impl->seats)
		{
			if (seat.kind != SeatComputer || !seat.bot)
				continue;
			seat.bot->leave_game();
			delete seat.bot;
			seat.bot = nullptr;
		}
		impl->bots.clear();
		for (BotBase *bot : impl->extra_bots)
		{
			bot->leave_game();
			delete bot;
		}
		impl->extra_bots.clear();
		impl->extra_bot_infos.clear();
		impl->rebuild_and_notify();
	});
}

void SessionCore::undo_move()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		GameClient *target = nullptr;
		if (impl->is_host)
			for (Seat &seat : impl->seats)
				if (seat.client && seat.client->ready())
				{
					target = seat.client;
					break;
				}
		if (!target)
			target = impl->active_client();
		if (!target)
			target = impl->client;
		if (target)
			target->undo_move();
	});
}

void SessionCore::restart_game()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		impl->perform_server_action([](GameServer *server) { server->restart_game(); });
	});
}

void SessionCore::rotate_players()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		impl->perform_server_action([](GameServer *server) { server->rotate_players(); });
	});
}

void SessionCore::shuffle_players()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		impl->perform_server_action([](GameServer *server) { server->shuffle_players(); });
	});
}

void SessionCore::tap_hole(int hole)
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl, hole]()
	{
		GameClient *active = impl->active_client();
		if (!active || !active->get_board() || active->is_spectator())
			return;
		if (impl->status == 0)
			return;
		int my = (int)active->get_my_player_number();
		if (my == 0)
			return;

		int current = (int)active->get_current_player();
		if (current != 0 && current != my)
			return;

		GameBoard *board = active->get_board();
		std::vector<unsigned int> &selection = impl->selection;
		auto it = std::find(selection.begin(), selection.end(), (unsigned int)hole);

		if (it == selection.end())
		{
			selection.push_back((unsigned int)hole);
			bool valid = false;
			if (selection.size() == 1)
			{
				GameHole *h = (*board)[(unsigned int)hole];
				valid = h && (int)h->get_current_player() == my;
			}
			else
			{
				valid = board->valid_move_list(selection, false);
			}

			if (!valid)
			{
				selection.pop_back();
				GameHole *h = (*board)[(unsigned int)hole];
				if (h && (int)h->get_current_player() == my)
				{
					selection.clear();
					selection.push_back((unsigned int)hole);
					active->show_move(&selection);
				}
			}
			else
				active->show_move(&selection);
		}
		else
		{
			if (!selection.empty() && selection.back() == (unsigned int)hole)
				selection.pop_back();
			else
				while (!selection.empty() && selection.back() != (unsigned int)hole)
					selection.pop_back();

			if (selection.empty())
				active->hide_move();
			else
				active->show_move(&selection);
		}

		impl->rebuild_and_notify();
	});
}

void SessionCore::confirm_move()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		GameClient *active = impl->active_client();
		if (!active)
			return;

		if (impl->selection.size() > 1)
			active->make_move(&impl->selection);

		impl->selection.clear();
		impl->rebuild_and_notify();
	});
}

void SessionCore::clear_selection()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		GameClient *active = impl->active_client();
		if (!active)
			return;

		if (!impl->selection.empty())
		{
			impl->selection.clear();
			active->hide_move();
		}
		impl->rebuild_and_notify();
	});
}

void SessionCore::remove_last_hop()
{
	auto impl = impl_;
	cheech::Loop::instance().post([impl]()
	{
		GameClient *active = impl->active_client();
		if (!active)
			return;

		if (!impl->selection.empty())
		{
			impl->selection.pop_back();
			if (impl->selection.empty())
				active->hide_move();
			else
				active->show_move(&impl->selection);
		}
		impl->rebuild_and_notify();
	});
}

Snapshot SessionCore::snapshot() const
{
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->snap;
}

bool SessionCore::consume_replay_skip()
{
	std::lock_guard<std::mutex> lock(impl_->mutex);
	bool skip = impl_->snap.skip_replay;
	impl_->snap.skip_replay = false;
	return skip;
}

void SessionCore::hold_computer_moves_for_animation_hops(int hops)
{
	if (hops < 0)
		return;
	std::lock_guard<std::mutex> lock(impl_->mutex);
	cheech::extend_move_gate(impl_->anim_step_ms * hops + impl_->anim_done_ms);
}

void SessionCore::shutdown()
{
	auto impl = impl_;
	impl_.reset();
	if (!impl)
		return;
	if (cheech::Loop::instance().running())
		cheech::Loop::instance().post([impl]() { impl->stop_core(); });
	else
		impl->stop_core();
}

namespace
{

int first_unused_color(GameClient *client)
{
	int chosen = 1;
	GameBoard *board = client ? client->get_board() : nullptr;
	if (board)
	{
		bool used[9] = {false};
		for (unsigned int p = 1; p <= 6; p++)
		{
			int c = (int)client->get_player_color(p);
			if (c >= 1 && c <= 8)
				used[c] = true;
		}
		for (int c = 1; c <= 8; c++)
			if (!used[c])
			{
				chosen = c;
				break;
			}
	}
	return chosen;
}

} // namespace

void SessionCore::Impl::connect_seat_signals(GameClient *seat_client, int index)
{
	seat_client->evt_connected.connect([seat_client]()
	{
		seat_client->request_player_ids();
	});

	seat_client->cmd_player_id.connect([this, seat_client, index](unsigned int posn, unsigned int id)
	{
		if (index >= 0 && index < (int)seats.size()
			&& posn == seat_client->get_my_player_number())
		{
			seats[index].player_id = id;
			seats[index].has_player_id = true;
		}
		apply_seat_focus(this);
		rebuild_and_notify();
	});

	seat_client->cmd_player_ids_end.connect([this]()
	{
		apply_seat_focus(this);
		rebuild_and_notify();
	});

	seat_client->cmd_set_player_number.connect([this, seat_client, index](unsigned int n)
	{
		for (auto it = client_by_number.begin(); it != client_by_number.end();)
			it = (it->second == seat_client) ? client_by_number.erase(it) : std::next(it);
		if (index >= 0 && index < (int)seats.size())
			seats[index].player_number = (int)n;
		client_by_number[(int)n] = seat_client;
		apply_seat_focus(this);
		rebuild_and_notify();
	});

	seat_client->cmd_choose_new_name.connect([seat_client, index](Glib::ustring base)
	{
		Glib::ustring new_name = base + " " + std::to_string(index + 1);
		seat_client->change_name(new_name);
	});

	seat_client->cmd_choose_new_color.connect([seat_client](Glib::ustring, int)
	{
		seat_client->change_color(first_unused_color(seat_client));
	});
}

void SessionCore::Impl::connect_display_signals()
{
	if (!client)
		return;
	GameClient *display = client;

	display->evt_connected.connect([this, display]()
	{
		status = 0;
		display->request_player_ids();
		rebuild_and_notify();
	});

	display->evt_disconnected.connect([this]() { rebuild_and_notify(); });
	display->evt_cancelled.connect([this]() { rebuild_and_notify(); });

	display->cmd_set_player_number.connect([this](unsigned int)
	{
		apply_seat_focus(this);
		apply_extra_bot_focus(this);
		rebuild_and_notify();
	});

	display->cmd_player_add.connect([this, display](unsigned int, Glib::ustring, int)
	{
		associate_remote_seats(this);
		display->request_player_ids();
		rebuild_and_notify();
	});

	display->cmd_player_remove.connect([this](unsigned int) { rebuild_and_notify(); });
	display->cmd_player_id.connect([this](unsigned int, unsigned int) { rebuild_and_notify(); });

	display->cmd_player_ids_end.connect([this, display]()
	{
		for (Seat &seat : seats)
			if (seat.kind == SeatRemote && seat.has_player_id && seat.player_id)
			{
				unsigned int posn = display->get_posn_for_id(seat.player_id);
				if (posn >= 1 && posn <= 6)
					seat.player_number = (int)posn;
			}
		associate_remote_seats(this);
		apply_seat_focus(this);
		apply_extra_bot_focus(this);
		rebuild_and_notify();
	});

	display->cmd_player_finish.connect([this](unsigned int posn, unsigned int move_count)
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (posn >= 1 && posn <= 6)
				snap.finished_in_moves[posn] = (int)move_count;
		}
		rebuild_and_notify();
	});

	display->cmd_game_resync.connect([this, display]()
	{
		display->request_player_ids();
		rebuild_and_notify();
	});

	display->cmd_game_turn.connect([this](unsigned int posn, GameServer::GameStatus game_status,
		unsigned int move_number)
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			status = (int)game_status;
			if (posn == 1 && move_number == 1)
			{
				for (int i = 0; i < 7; i++)
				{
					snap.finished_in_moves[i] = 0;
					snap.moves_taken[i] = 0;
				}
				snap.move_number = 0;
			}
		}
		rebuild_and_notify();
	});

	display->cmd_game_show_move.connect([this](MoveList *move)
	{
		if (!move)
		{
			rebuild_and_notify();
			return;
		}

		int owner = 0;
		if (!move->empty() && client)
		{
			GameBoard *board = client->get_board();
			if (board && (*board)[(*move)[0]])
				owner = (int)(*board)[(*move)[0]]->get_current_player();
		}

		if (owner != 0 && owner == (int)client->get_my_player_number())
		{
			{
				std::lock_guard<std::mutex> lock(mutex);
				selection = *move;
			}
			rebuild_and_notify();
		}
		else if (owner != 0 && client_by_number.count((unsigned int)owner) == 0)
		{
			{
				std::lock_guard<std::mutex> lock(mutex);
				snap.telegraph.assign(move->begin(), move->end());
			}
			rebuild_and_notify();
		}
	});

	display->cmd_game_hide_move.connect([this]()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			selection.clear();
			snap.telegraph.clear();
		}
		rebuild_and_notify();
	});

	display->cmd_game_make_move.connect([this](MoveList *move)
	{
		if (move && !move->empty())
		{
			std::lock_guard<std::mutex> lock(mutex);
			snap.last_move.assign(move->begin(), move->end());
			snap.move_serial++;
			snap.move_number++;
			selection.clear();

			snap.skip_replay =
				snap.telegraph.size() == move->size() &&
				std::equal(snap.telegraph.begin(), snap.telegraph.end(), move->begin());
			snap.telegraph.clear();

			if (client)
			{
				GameBoard *board = client->get_board();
				int mover = (board && (*board)[move->back()])
					? (int)(*board)[move->back()]->get_current_player() : 0;
				if (mover >= 1 && mover <= 6)
					snap.moves_taken[mover]++;

				if (!snap.skip_replay && mover != (int)client->get_my_player_number())
					cheech::extend_move_gate(anim_step_ms * ((int)move->size() - 1)
						+ anim_done_ms);
			}
		}
		rebuild_and_notify();
	});

	display->cmd_game_undo_move.connect([this](unsigned int from, unsigned int)
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (snap.move_number > 0)
				snap.move_number--;
			if (client)
			{
				GameBoard *board = client->get_board();
				int mover = (board && (*board)[from])
					? (int)(*board)[from]->get_current_player() : 0;
				if (mover >= 1 && mover <= 6 && snap.moves_taken[mover] > 0)
					snap.moves_taken[mover]--;
			}
		}
		rebuild_and_notify();
	});

	display->evt_message.connect([this](Glib::ustring message)
	{
		notify_message(std::string(message));
	});

	display->cmd_choose_new_name.connect([this](Glib::ustring name)
	{
		if (!client)
			return;
		name_counter++;
		Glib::ustring new_name = name + " " + std::to_string(name_counter + 1);
		client->change_name(new_name);
		rebuild_and_notify();
	});

	display->cmd_choose_new_color.connect([this](Glib::ustring, int)
	{
		if (!client)
			return;
		client->change_color(first_unused_color(client));
		rebuild_and_notify();
	});
}

void SessionCore::Impl::rebuild_snapshot()
{
	std::lock_guard<std::mutex> lock(mutex);
	Snapshot &s = snap;

	s.connected = false;
	s.is_host = is_host;
	s.is_spectator = is_spectator;
	s.active_seat_kind = SeatUnassigned;
	s.has_local_human_seat = !client_by_number.empty();
	s.has_remote_seat = false;
	for (const Seat &seat : seats)
		if (seat.kind == SeatRemote)
		{
			s.has_remote_seat = true;
			break;
		}
	s.local_human_player_number = (client_by_number.size() == 1)
		? client_by_number.begin()->first : 0;
	s.status = status;
	s.animation_step_ms = anim_step_ms;
	s.animation_done_ms = anim_done_ms;
	s.computer_smarts = computer_smarts;
	s.current_player = 0;
	s.my_player_number = 0;
	s.num_players = configured_players;
	s.server_host.clear();
	s.server_port = 0;
	s.long_jumps = true;
	s.hop_others = true;
	s.stop_others = true;
	s.extra_computer_player_count = (int)extra_bots.size();
	s.computer_player_count = (int)bots.size() + (int)extra_bots.size();
	for (PlayerInfo &player : s.players)
		player = PlayerInfo();
	s.player_count = 0;
	std::fill(std::begin(s.holes), std::end(s.holes), 0);
	std::fill(std::begin(s.finished), std::end(s.finished), false);
	std::fill(std::begin(s.pegs_in_goal), std::end(s.pegs_in_goal), 0);
	s.web_port = (int)web_port;
	s.web_ready = web_server != nullptr;

	if (!client)
	{
		s.selected.clear();
		s.telegraph.clear();
		return;
	}

	s.connected = client->ready();
	s.my_player_number = (int)client->get_my_player_number();
	s.current_player = (int)client->get_current_player();
	s.is_spectator = is_host ? false : client->is_spectator();
	s.server_host = client->get_host_name();
	s.server_port = (int)client->get_port();

	GameBoard *board = client->get_board();
	if (board)
	{
		s.num_players = (int)board->get_num_players();
		s.long_jumps = board->get_long_jumps_allowed();
		s.hop_others = board->get_hop_others_allowed();
		s.stop_others = board->get_stop_others_allowed();
	}

	for (unsigned int p = 1; p <= 6; p++)
	{
		Glib::ustring name = client->get_player_name(p);
		if (!name.empty())
		{
			s.players[p].name = name;
			s.players[p].color = (int)client->get_player_color(p);
			s.player_count++;
		}
	}

	if (board)
	{
		for (unsigned int i = 0; i < GameBoard::SIZE; i++)
		{
			GameHole *h = (*board)[i];
			s.holes[i] = h ? (int)h->get_current_player() : 0;
		}

		for (unsigned int p = 1; p <= 6; p++)
		{
			s.finished[p] = board->player_finished(p);
			s.pegs_in_goal[p] = (int)board->get_num_pegs_in_goal(p);
		}
	}

	bool fully_local = is_host && server && board && configured_players > 0;
	if (fully_local)
		for (const Seat &seat : seats)
			if (seat.kind == SeatRemote)
			{
				fully_local = false;
				break;
			}
	if (fully_local && (s.player_count < s.num_players || board->game_finished()))
		fully_local = false;
	saved_state = fully_local ? build_local_save(this) : std::string();

	if (is_host)
	{
		int current = (int)client->get_current_player();
		if (current >= 1)
		{
			if (client_by_number.find(current) != client_by_number.end())
				s.active_seat_kind = SeatHuman;
			else
			{
				bool is_computer = false;
				for (BotBase *bot : bots)
				{
					GameClient *bot_client = bot->get_game_client();
					if (bot_client && (int)bot_client->get_my_player_number() == current)
					{
						is_computer = true;
						break;
					}
				}
				if (is_computer)
					s.active_seat_kind = SeatComputer;
				else if (!client->get_player_name(current).empty())
					s.active_seat_kind = SeatRemote;
			}
		}
	}

	s.selected.assign(selection.begin(), selection.end());
}

void SessionCore::Impl::rebuild_and_notify()
{
	rebuild_snapshot();
	notify_update();
}

} // namespace mobile
} // namespace cheech
