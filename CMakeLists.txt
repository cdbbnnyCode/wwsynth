cmake_minimum_required(VERSION 3.3)

project(synth)

set(CXX_STANDARD 11)
set(COMPILE_FLAGS -g -Og -Wall -Wextra)

include_directories(src/ /usr/include)

add_executable(synth
    src/banks.cpp
    src/util.cpp
    src/aaf.cpp
    src/instrument.cpp
    src/audio_system.cpp
    src/main.cpp
)


target_link_libraries(synth /usr/lib/libstk.so)
