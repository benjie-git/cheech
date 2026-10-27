#include <jni.h>

#include <algorithm>
#include <csignal>
#include <codecvt>
#include <locale>
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

JavaVM *g_vm = nullptr;
jclass g_snapshot_class = nullptr;
jmethodID g_snapshot_ctor = nullptr;
jclass g_string_class = nullptr;
jclass g_seat_class = nullptr;

jfieldID g_seat_kind = nullptr;
jfieldID g_seat_bot_type = nullptr;
jfieldID g_seat_name = nullptr;
jfieldID g_seat_color = nullptr;
jfieldID g_seat_smarts = nullptr;
jfieldID g_seat_friend_seats = nullptr;
jfieldID g_seat_enemy_seats = nullptr;

constexpr int kPlayerBase = 32;
constexpr int kPlayerStride = 5;
constexpr int kHoleBase = 62;
constexpr int kSelectedBase = kHoleBase + 247;
constexpr int kTelegraphBase = kSelectedBase + 64;
constexpr int kLastMoveBase = kTelegraphBase + 64;
constexpr int kSnapshotInts = kLastMoveBase + 64;

struct NativeSession
{
	SessionCore *core = nullptr;
	jobject listener = nullptr;
	jmethodID on_snapshot = nullptr;
	jmethodID on_message = nullptr;
};

std::u16string utf8_to_utf16(const std::string &input)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	try
	{
		return convert.from_bytes(input);
	}
	catch (const std::range_error &)
	{
		return std::u16string();
	}
}

std::string utf16_to_utf8(const jchar *chars, jsize length)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	try
	{
		std::u16string utf16(reinterpret_cast<const char16_t *>(chars),
			(size_t)length);
		return convert.to_bytes(utf16);
	}
	catch (const std::range_error &)
	{
		return std::string();
	}
}

std::string jstring_to_utf8(JNIEnv *env, jstring value)
{
	if (!value)
		return std::string();
	const jchar *chars = env->GetStringChars(value, nullptr);
	jsize length = env->GetStringLength(value);
	std::string result = utf16_to_utf8(chars, length);
	env->ReleaseStringChars(value, chars);
	return result;
}

jstring utf8_to_jstring(JNIEnv *env, const std::string &value)
{
	std::u16string utf16 = utf8_to_utf16(value);
	return env->NewString(reinterpret_cast<const jchar *>(utf16.data()),
		(jsize)utf16.size());
}

JNIEnv *attach_env(bool &attached)
{
	attached = false;
	JNIEnv *env = nullptr;
	jint result = g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
	if (result == JNI_EDETACHED)
	{
#ifdef __ANDROID__
		if (g_vm->AttachCurrentThreadAsDaemon(&env, nullptr) != JNI_OK)
			return nullptr;
#else
		if (g_vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void **>(&env),
				nullptr) != JNI_OK)
			return nullptr;
#endif
		attached = true;
	}
	else if (result != JNI_OK)
		return nullptr;
	return env;
}

jobject build_snapshot(JNIEnv *env, const Snapshot &snapshot)
{
	std::vector<jint> values(kSnapshotInts, 0);
	values[0] = snapshot.connected ? 1 : 0;
	values[1] = snapshot.is_host ? 1 : 0;
	values[2] = snapshot.is_spectator ? 1 : 0;
	values[3] = snapshot.active_seat_kind;
	values[4] = snapshot.has_local_human_seat ? 1 : 0;
	values[5] = snapshot.has_remote_seat ? 1 : 0;
	values[6] = snapshot.local_human_player_number;
	values[7] = snapshot.status;
	values[8] = snapshot.num_players;
	values[9] = snapshot.my_player_number;
	values[10] = snapshot.current_player;
	values[11] = snapshot.player_count;
	values[12] = snapshot.server_port;
	values[13] = snapshot.long_jumps ? 1 : 0;
	values[14] = snapshot.hop_others ? 1 : 0;
	values[15] = snapshot.stop_others ? 1 : 0;
	values[16] = snapshot.extra_computer_player_count;
	values[17] = snapshot.computer_player_count;
	values[18] = (jint)snapshot.move_serial;
	values[19] = snapshot.move_number;
	values[20] = snapshot.animation_step_ms;
	values[21] = snapshot.animation_done_ms;
	values[22] = snapshot.computer_smarts;
	values[23] = snapshot.web_port;
	values[24] = snapshot.web_ready ? 1 : 0;
	values[25] = (jint)std::min<size_t>(snapshot.selected.size(), 64);
	values[26] = (jint)std::min<size_t>(snapshot.telegraph.size(), 64);
	values[27] = (jint)std::min<size_t>(snapshot.last_move.size(), 64);

	for (int p = 1; p <= 6; p++)
	{
		int base = kPlayerBase + (p - 1) * kPlayerStride;
		values[base] = snapshot.players[p].color;
		values[base + 1] = snapshot.finished[p] ? 1 : 0;
		values[base + 2] = snapshot.finished_in_moves[p];
		values[base + 3] = snapshot.moves_taken[p];
		values[base + 4] = snapshot.pegs_in_goal[p];
	}

	for (int i = 0; i < 247; i++)
		values[kHoleBase + i] = snapshot.holes[i];
	for (size_t i = 0; i < (size_t)values[25]; i++)
		values[kSelectedBase + i] = snapshot.selected[i];
	for (size_t i = 0; i < (size_t)values[26]; i++)
		values[kTelegraphBase + i] = snapshot.telegraph[i];
	for (size_t i = 0; i < (size_t)values[27]; i++)
		values[kLastMoveBase + i] = snapshot.last_move[i];

	jintArray ints = env->NewIntArray(kSnapshotInts);
	env->SetIntArrayRegion(ints, 0, kSnapshotInts, values.data());

	jobjectArray names = env->NewObjectArray(7, g_string_class, nullptr);
	for (int p = 1; p <= 6; p++)
	{
		jstring name = utf8_to_jstring(env, snapshot.players[p].name);
		env->SetObjectArrayElement(names, p, name);
		env->DeleteLocalRef(name);
	}

	jstring host = utf8_to_jstring(env, snapshot.server_host);
	jobject result = env->NewObject(g_snapshot_class, g_snapshot_ctor,
		ints, names, host);
	env->DeleteLocalRef(ints);
	env->DeleteLocalRef(names);
	env->DeleteLocalRef(host);
	return result;
}

void post_snapshot(NativeSession *session, const Snapshot &snapshot)
{
	bool attached = false;
	JNIEnv *env = attach_env(attached);
	if (!env)
		return;

	jobject object = build_snapshot(env, snapshot);
	if (object)
	{
		env->CallVoidMethod(session->listener, session->on_snapshot, object);
		env->DeleteLocalRef(object);
	}
	if (env->ExceptionCheck())
		env->ExceptionClear();
	if (attached)
		g_vm->DetachCurrentThread();
}

void post_message(NativeSession *session, const std::string &message)
{
	bool attached = false;
	JNIEnv *env = attach_env(attached);
	if (!env)
		return;

	jstring text = utf8_to_jstring(env, message);
	env->CallVoidMethod(session->listener, session->on_message, text);
	env->DeleteLocalRef(text);
	if (env->ExceptionCheck())
		env->ExceptionClear();
	if (attached)
		g_vm->DetachCurrentThread();
}

std::optional<std::vector<int>> read_int_array(JNIEnv *env, jintArray array)
{
	if (!array)
		return std::nullopt;
	jsize length = env->GetArrayLength(array);
	std::vector<int> result((size_t)length);
	if (length > 0)
	{
		std::vector<jint> values((size_t)length);
		env->GetIntArrayRegion(array, 0, length, values.data());
		for (jsize i = 0; i < length; i++)
			result[(size_t)i] = values[(size_t)i];
	}
	return result;
}

std::vector<SeatSpec> read_seats(JNIEnv *env, jobjectArray seats)
{
	std::vector<SeatSpec> result;
	if (!seats)
		return result;

	jsize count = env->GetArrayLength(seats);
	result.reserve((size_t)count);
	for (jsize i = 0; i < count; i++)
	{
		jobject seat = env->GetObjectArrayElement(seats, i);
		if (!seat)
			continue;
		SeatSpec spec;
		spec.kind = env->GetIntField(seat, g_seat_kind);
		auto bot_type = reinterpret_cast<jstring>(env->GetObjectField(seat, g_seat_bot_type));
		auto name = reinterpret_cast<jstring>(env->GetObjectField(seat, g_seat_name));
		spec.bot_type = jstring_to_utf8(env, bot_type);
		spec.name = jstring_to_utf8(env, name);
		spec.color = env->GetIntField(seat, g_seat_color);
		spec.smarts = env->GetIntField(seat, g_seat_smarts);
		auto friends = reinterpret_cast<jintArray>(env->GetObjectField(seat, g_seat_friend_seats));
		auto enemies = reinterpret_cast<jintArray>(env->GetObjectField(seat, g_seat_enemy_seats));
		spec.friend_seats = read_int_array(env, friends);
		spec.enemy_seats = read_int_array(env, enemies);
		env->DeleteLocalRef(bot_type);
		env->DeleteLocalRef(name);
		env->DeleteLocalRef(friends);
		env->DeleteLocalRef(enemies);
		env->DeleteLocalRef(seat);
		result.push_back(spec);
	}
	return result;
}

NativeSession *native_session(jlong handle)
{
	return reinterpret_cast<NativeSession *>(handle);
}

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *)
{
	g_vm = vm;
	JNIEnv *env = nullptr;
	if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
		return JNI_ERR;

	signal(SIGPIPE, SIG_IGN);

	jclass snapshot_class = env->FindClass(
		"com/octopilot/cheech/bridge/CheechSnapshot");
	if (!snapshot_class)
		return JNI_ERR;
	g_snapshot_class = reinterpret_cast<jclass>(env->NewGlobalRef(snapshot_class));
	env->DeleteLocalRef(snapshot_class);
	g_snapshot_ctor = env->GetMethodID(g_snapshot_class, "<init>",
		"([I[Ljava/lang/String;Ljava/lang/String;)V");

	g_string_class = reinterpret_cast<jclass>(env->NewGlobalRef(
		env->FindClass("java/lang/String")));

	jclass seat_class = env->FindClass(
		"com/octopilot/cheech/bridge/CheechSeatSpec");
	if (!seat_class)
		return JNI_ERR;
	g_seat_class = reinterpret_cast<jclass>(env->NewGlobalRef(seat_class));
	g_seat_kind = env->GetFieldID(seat_class, "kind", "I");
	g_seat_bot_type = env->GetFieldID(seat_class, "botType", "Ljava/lang/String;");
	g_seat_name = env->GetFieldID(seat_class, "name", "Ljava/lang/String;");
	g_seat_color = env->GetFieldID(seat_class, "color", "I");
	g_seat_smarts = env->GetFieldID(seat_class, "smarts", "I");
	g_seat_friend_seats = env->GetFieldID(seat_class, "friendSeats", "[I");
	g_seat_enemy_seats = env->GetFieldID(seat_class, "enemySeats", "[I");
	env->DeleteLocalRef(seat_class);

	jclass session_class = env->FindClass(
		"com/octopilot/cheech/bridge/CheechSession");
	if (!session_class)
		return JNI_ERR;

	static const JNINativeMethod methods[] = {
		{"nativeCreate", "()J", (void *)+[](JNIEnv *, jobject) -> jlong {
			auto *session = new NativeSession();
			session->core = new SessionCore();
			return reinterpret_cast<jlong>(session);
		}},
		{"nativeDestroy", "(J)V", (void *)+[](JNIEnv *env, jobject, jlong handle) {
			auto *session = native_session(handle);
			if (!session)
				return;
			session->core->set_handlers(nullptr, nullptr);
			session->core->shutdown();
			delete session->core;
			if (session->listener)
				env->DeleteGlobalRef(session->listener);
			delete session;
		}},
		{"nativeSetListener", "(JLcom/octopilot/cheech/bridge/CheechListener;)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jobject listener) {
				auto *session = native_session(handle);
				if (!session)
					return;
				if (session->listener)
					env->DeleteGlobalRef(session->listener);
				session->listener = env->NewGlobalRef(listener);
				jclass cls = env->GetObjectClass(listener);
				session->on_snapshot = env->GetMethodID(cls, "onSnapshot",
					"(Lcom/octopilot/cheech/bridge/CheechSnapshot;)V");
				session->on_message = env->GetMethodID(cls, "onMessage",
					"(Ljava/lang/String;)V");
				env->DeleteLocalRef(cls);
				session->core->set_handlers(
					[session](const Snapshot &snapshot) { post_snapshot(session, snapshot); },
					[session](const std::string &message) { post_message(session, message); });
			}},
		{"nativeJoinHost", "(JLjava/lang/String;IZLjava/lang/String;I)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jstring host, jint port,
				jboolean spectator, jstring name, jint color) {
				auto *session = native_session(handle);
				if (session)
					session->core->join_host(jstring_to_utf8(env, host), (unsigned int)port,
						spectator == JNI_TRUE, jstring_to_utf8(env, name), color);
			}},
		{"nativeStartGame",
			"(JIIZZZ[Lcom/octopilot/cheech/bridge/CheechSeatSpec;I)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jint port, jint num_players,
				jboolean long_jumps, jboolean hop_others, jboolean stop_others,
				jobjectArray seats, jint web_port) {
				auto *session = native_session(handle);
				if (session)
					session->core->start_game((unsigned int)port, num_players,
						long_jumps == JNI_TRUE, hop_others == JNI_TRUE,
						stop_others == JNI_TRUE, read_seats(env, seats),
						(unsigned int)web_port);
			}},
		{"nativeResumeLocalGame", "(JLjava/lang/String;I)Z",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jstring save, jint web_port) -> jboolean {
				auto *session = native_session(handle);
				if (!session)
					return JNI_FALSE;
				return session->core->resume_local_game(jstring_to_utf8(env, save),
					(unsigned int)web_port) ? JNI_TRUE : JNI_FALSE;
			}},
		{"nativeLocalGameSave", "(J)Ljava/lang/String;",
			(void *)+[](JNIEnv *env, jobject, jlong handle) -> jstring {
				auto *session = native_session(handle);
				if (!session)
					return nullptr;
				std::string save = session->core->local_game_save();
				return save.empty() ? nullptr : utf8_to_jstring(env, save);
			}},
		{"nativeLeave", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->leave();
		}},
		{"nativeChangeName", "(JLjava/lang/String;)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jstring name) {
				if (auto *session = native_session(handle))
					session->core->change_name(jstring_to_utf8(env, name));
			}},
		{"nativeChangeColor", "(JI)V", (void *)+[](JNIEnv *, jobject, jlong handle, jint color) {
			if (auto *session = native_session(handle))
				session->core->change_color(color);
		}},
		{"nativeSendChat", "(JLjava/lang/String;)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jstring text) {
				if (auto *session = native_session(handle))
					session->core->send_chat(jstring_to_utf8(env, text));
			}},
		{"nativeSetAnimationStepMs", "(JI)V", (void *)+[](JNIEnv *, jobject, jlong handle, jint ms) {
			if (auto *session = native_session(handle))
				session->core->set_animation_step_ms(ms);
		}},
		{"nativeSetComputerSmarts", "(JI)V", (void *)+[](JNIEnv *, jobject, jlong handle, jint percent) {
			if (auto *session = native_session(handle))
				session->core->set_computer_smarts(percent);
		}},
		{"nativeReconfigureGame", "(JIZZZ)V",
			(void *)+[](JNIEnv *, jobject, jlong handle, jint players, jboolean long_jumps,
				jboolean hop_others, jboolean stop_others) {
				if (auto *session = native_session(handle))
					session->core->reconfigure_game(players, long_jumps == JNI_TRUE,
						hop_others == JNI_TRUE, stop_others == JNI_TRUE);
			}},
		{"nativeAddComputerPlayer", "(JLjava/lang/String;Ljava/lang/String;I[I[I)V",
			(void *)+[](JNIEnv *env, jobject, jlong handle, jstring type, jstring name,
				jint color, jintArray friends, jintArray enemies) {
				if (auto *session = native_session(handle))
					session->core->add_computer_player(jstring_to_utf8(env, type),
						jstring_to_utf8(env, name), color,
						read_int_array(env, friends), read_int_array(env, enemies));
			}},
		{"nativeRemoveComputerPlayers", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->remove_computer_players();
		}},
		{"nativeUndoMove", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->undo_move();
		}},
		{"nativeRestartGame", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->restart_game();
		}},
		{"nativeRotatePlayers", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->rotate_players();
		}},
		{"nativeShufflePlayers", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->shuffle_players();
		}},
		{"nativeTapHole", "(JI)V", (void *)+[](JNIEnv *, jobject, jlong handle, jint hole) {
			if (auto *session = native_session(handle))
				session->core->tap_hole(hole);
		}},
		{"nativeConfirmMove", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->confirm_move();
		}},
		{"nativeClearSelection", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->clear_selection();
		}},
		{"nativeRemoveLastHop", "(J)V", (void *)+[](JNIEnv *, jobject, jlong handle) {
			if (auto *session = native_session(handle))
				session->core->remove_last_hop();
		}},
		{"nativeConsumeReplaySkip", "(J)Z", (void *)+[](JNIEnv *, jobject, jlong handle) -> jboolean {
			if (auto *session = native_session(handle))
				return session->core->consume_replay_skip() ? JNI_TRUE : JNI_FALSE;
			return JNI_FALSE;
		}},
		{"nativeHoldComputerMoves", "(JI)V", (void *)+[](JNIEnv *, jobject, jlong handle, jint hops) {
			if (auto *session = native_session(handle))
				session->core->hold_computer_moves_for_animation_hops(hops);
		}},
		{"boardHoleCount", "()I", (void *)+[](JNIEnv *, jclass) -> jint {
			return SessionCore::board_hole_count();
		}},
		{"boardHolePresent", "()[Z", (void *)+[](JNIEnv *env, jclass) -> jbooleanArray {
			jbooleanArray array = env->NewBooleanArray(SessionCore::board_hole_count());
			std::vector<jboolean> values((size_t)SessionCore::board_hole_count());
			for (int i = 0; i < SessionCore::board_hole_count(); i++)
				values[(size_t)i] = SessionCore::board_hole_present(i) ? JNI_TRUE : JNI_FALSE;
			env->SetBooleanArrayRegion(array, 0, (jsize)values.size(), values.data());
			return array;
		}},
		{"boardHoleX", "()[D", (void *)+[](JNIEnv *env, jclass) -> jdoubleArray {
			jdoubleArray array = env->NewDoubleArray(SessionCore::board_hole_count());
			std::vector<jdouble> values((size_t)SessionCore::board_hole_count());
			for (int i = 0; i < SessionCore::board_hole_count(); i++)
				values[(size_t)i] = SessionCore::board_hole_x(i);
			env->SetDoubleArrayRegion(array, 0, (jsize)values.size(), values.data());
			return array;
		}},
		{"boardHoleY", "()[D", (void *)+[](JNIEnv *env, jclass) -> jdoubleArray {
			jdoubleArray array = env->NewDoubleArray(SessionCore::board_hole_count());
			std::vector<jdouble> values((size_t)SessionCore::board_hole_count());
			for (int i = 0; i < SessionCore::board_hole_count(); i++)
				values[(size_t)i] = SessionCore::board_hole_y(i);
			env->SetDoubleArrayRegion(array, 0, (jsize)values.size(), values.data());
			return array;
		}},
		{"defaultNameForComputerType", "(Ljava/lang/String;)Ljava/lang/String;",
			(void *)+[](JNIEnv *env, jclass, jstring type) -> jstring {
				return utf8_to_jstring(env,
					SessionCore::default_name_for_computer_type(jstring_to_utf8(env, type)));
			}},
		{"typeNameForComputerType", "(Ljava/lang/String;)Ljava/lang/String;",
			(void *)+[](JNIEnv *env, jclass, jstring type) -> jstring {
				return utf8_to_jstring(env,
					SessionCore::type_name_for_computer_type(jstring_to_utf8(env, type)));
			}},
		{"setWebRoot", "(Ljava/lang/String;)V",
			(void *)+[](JNIEnv *env, jclass, jstring path) {
				SessionCore::set_web_root(jstring_to_utf8(env, path));
			}},
	};

	if (env->RegisterNatives(session_class, methods,
			sizeof(methods) / sizeof(methods[0])) != JNI_OK)
		return JNI_ERR;
	env->DeleteLocalRef(session_class);

	return JNI_VERSION_1_6;
}
