cmake_minimum_required(VERSION 3.15)
project(minifs C)

set(CMAKE_C_STANDARD 99)

set(LOCAL_MAIN run_local.c)
set(SERVER_MAIN run_server.c)
set(CLIENT_MAIN run_client.c)

# libraries
set(LIB_FS src/fs.c)
set(LIB_TERMINAL src/terminal.c)
set(LIB_SERVER src/server.c)
set(LIB_CLIENT src/client.c)

include_directories(headers)

add_library(libfs STATIC ${LIB_FS})
add_library(libterminal STATIC ${LIB_TERMINAL})
add_library(libserver STATIC ${LIB_SERVER})
add_library(libclient STATIC ${LIB_CLIENT})


add_executable(local ${LOCAL_MAIN})
add_executable(server ${SERVER_MAIN})
add_executable(client ${CLIENT_MAIN})

target_link_libraries(libterminal libfs)

target_link_libraries(local libfs)
target_link_libraries(local libterminal)

target_link_libraries(libserver libfs)
target_link_libraries(libserver libterminal)
target_link_libraries(server libserver)

target_link_libraries(client libclient)
