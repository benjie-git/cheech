# Cheech for Android

A native Kotlin + Jetpack Compose client for the shared cheech C++ engine. It
is feature-matched with the iOS app and can host, join, spectate, play against
computers, chat, save/resume local games, and serve the browser client.

## Requirements

- JDK 17 or newer (JDK 21 is used for development)
- Android SDK platform 35
- Android Build-Tools 35.0.1
- Android NDK 27.2.12479018
- CMake 3.22.1
- A device or emulator running Android 8.0 (API 26) or newer

The Gradle wrapper is checked in. If your SDK is not in the default location,
create `android/local.properties` (ignored by git):

```properties
sdk.dir=/path/to/Android/sdk
```

Open the `android/` directory itself in Android Studio, not the repository
root.

## Build and run

```bash
cd android
./gradlew assembleDebug
./gradlew installDebug
adb shell am start -n com.octopilot.cheech/.MainActivity
```

Run the checks:

```bash
./gradlew testDebugUnitTest
./gradlew lintDebug
./gradlew assembleRelease bundleRelease
```

Release builds are unsigned by default; configure a signing key before
publishing.

## Playing

- **Host**: choose the number of players, rules, and seats, then Start Game.
  A seat can be **Human** (on this device), **Computer**, or **Remote** (left
  open for another phone or the web client).
- **Join**: enter a host and port. Joining as a spectator is supported.
- **Invite**: while hosting, open Invite to share
  `cheech://join?host=<ip>&port=<port>`. The link opens the app directly.
- **Web**: while hosting, the app also serves the browser client on port 3839,
  bridging it to the game server on 3838. On the same device:

  ```bash
  adb forward tcp:3839 tcp:3839
  # open http://localhost:3839/
  ```

  On a LAN, open `http://<android-device-ip>:3839/`.
- **Save/resume**: a fully local hosted game (no remote seats) is saved when
  the app leaves the foreground and restored on the next launch.
- **Keyboard**: Enter commits the current move path, Escape cancels it, and
  Ctrl-Z undoes a move (host only). Shortcuts are ignored while chat or a
  dialog has focus.

The Android host is a standard cheech server, so iOS, desktop, and headless
clients can connect to port 3838. The Android client can likewise join any
existing cheech server.

## Architecture

```text
Compose UI (Kotlin)
  SessionModel / Prefs / SeatConfig / BotCatalog
    CheechSession (JNI wrapper)
      cheech_jni.cc
        src/mobile/cheech_session_core
          C++ core + ios/compat POSIX shims
```

- `src/mobile/cheech_session_core.{hh,cc}` is shared with iOS. It owns the
  in-process server, local/remote clients, bots, snapshot rebuilding, local
  save format, and web-host startup. It runs on `cheech::Loop`.
- `android/app/src/main/cpp/cheech_jni.cc` marshals seats, commands, and the
  immutable snapshot across JNI. Updates are delivered on the loop thread and
  posted to the Android main thread.
- `ios/compat/` provides the GLib/glibmm/sigc++/giomm subset and the POSIX
  event loop used by the mobile core. Both apps reuse it; there is no
  duplicated game logic.
- `android/app/src/main/assets/web/` is the checked-in browser client. It is
  copied to the app's files directory at startup and served by the embedded
  AJAX web server.

## Feature parity

- Local, computer, remote, and spectator play
- Bot types, skill levels, depth, and per-seat friend/enemy focus
- Board selection, multi-hop paths, animation, turn/winner display
- Chat, profile name/color, invite link and sharing
- Host controls: undo, restart, rotate, shuffle, reconfigure, add/remove bots,
  play again
- Fullscreen board, rotation, deep links, save/resume
- Embedded web client on port 3839

## Notes

- Android 16 introduced opt-in local-network protection; Android 17 enforces
  it for apps targeting SDK 37+. This app targets SDK 35, so `INTERNET` is
  sufficient for LAN play. A future target-37 update must add the
  `ACCESS_LOCAL_NETWORK` runtime permission.
- The app requests only `INTERNET`, `ACCESS_NETWORK_STATE`, and
  `ACCESS_WIFI_STATE`. It does not collect analytics or upload game data.
  See [PRIVACY.txt](PRIVACY.txt).
- If the build environment cannot write the default debug keystore location,
  point `ANDROID_USER_HOME` at a writable directory.
- The Android web host does not include a separate HTTP daemon; it is embedded
  in the app and starts with a hosted game.

## License

The shared engine is GPL-2.0-or-later; the app/bridge code follows the
repository's MIT license. See the root `LICENSE` and the per-file headers.
