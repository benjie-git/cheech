cmake_minimum_required(VERSION 3.22.1)
project(cheech LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CHEECH_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(CHEECH_SRC "${CHEECH_ROOT}/src")
set(CHEECH_COMPAT "${CHEECH_ROOT}/ios/compat")

add_library(cheech SHARED
    ${CHEECH_SRC}/game_board.cc
    ${CHEECH_SRC}/game_hole.cc
    ${CHEECH_SRC}/game_server.cc
    ${CHEECH_SRC}/game_client.cc
    ${CHEECH_SRC}/net_conn.cc
    ${CHEECH_SRC}/net_server.cc
    ${CHEECH_SRC}/utility.cc
    ${CHEECH_SRC}/bot_base.cc
    ${CHEECH_SRC}/bot_random.cc
    ${CHEECH_SRC}/bot_simple.cc
    ${CHEECH_SRC}/bot_lookahead.cc
    ${CHEECH_SRC}/bot_friendly.cc
    ${CHEECH_SRC}/bot_mean.cc
    ${CHEECH_SRC}/mobile/cheech_session_core.cc
    ${CHEECH_SRC}/ajax_server.cc
    ${CHEECH_SRC}/ajax_server_conn.cc
    ${CHEECH_COMPAT}/cheech_loop.cc
    ${CHEECH_COMPAT}/giomm/giomm.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cheech_jni.cc
)

target_include_directories(cheech PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CHEECH_SRC}
    ${CHEECH_COMPAT}
)

target_compile_definitions(cheech PRIVATE
    CHEECH_IOS=1
    CHEECH_ANDROID=1
)

target_compile_options(cheech PRIVATE
    -fexceptions
    -frtti
    -Wall
    -Wextra
    -Wno-unused-parameter
    # The bots are CPU-bound; keep debug builds representative instead of
    # running the shared core at -O0.
    $<$<CONFIG:Debug>:-O2>
)

# Match the iOS/desktop release builds: optimize across translation units.
# CMake's check_ipo_supported() probes with -fuse-ld=gold, which the NDK does
# not ship, so use clang's ThinLTO directly with the NDK's lld.
target_compile_options(cheech PRIVATE
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
)
target_link_options(cheech PRIVATE
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
    $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-fuse-ld=lld>
    "-Wl,-z,max-page-size=16384"
)
target_link_libraries(cheech PRIVATE log android)
