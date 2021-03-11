cmake_minimum_required(VERSION 3.17)
project(minifs C)

set(CMAKE_C_STANDARD 99)

set(MAIN main.c)

# libraries
set(LIB_FS src/fs.c)
set(LIB_TERMINAL src/terminal.c)

include_directories(headers)

add_library(fs STATIC ${LIB_FS})
add_library(terminal STATIC ${LIB_TERMINAL})


add_executable(minifs ${MAIN})

target_link_libraries(terminal fs)
target_link_libraries(minifs fs)
target_link_libraries(minifs terminal)
