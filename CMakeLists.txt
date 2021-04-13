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
    src/seq/parser.cpp
    src/seq/track.cpp
    src/recorder.cpp
)

add_executable(disassembler
    src/seq/parser.cpp
    src/disassembler.cpp
)

add_executable(player
    src/banks.cpp
    src/util.cpp
    src/aaf.cpp
    src/instrument.cpp
    src/audio_system.cpp
    src/seq/parser.cpp
    src/seq/track.cpp
    src/audio/audio_out.cpp
    src/player.cpp    
)

target_link_libraries(synth stk)
target_link_libraries(player stk SDL2)
