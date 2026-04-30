cmake_minimum_required(VERSION 4.2)
project(Cadence)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CADENCE_SRC
    src/scheduler.cpp
    src/worker.cpp
)

add_library(Cadence_lib
    STATIC
    ${CADENCE_SRC}
)

target_include_directories(Cadence_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(example
    examples/example.cpp
)

target_link_libraries(example
    PUBLIC
    Cadence_lib
)