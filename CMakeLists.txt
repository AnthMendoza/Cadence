cmake_minimum_required(VERSION 4.2)
project(Cadence)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

set(CADENCE_SRC
    src/scheduler.cpp
    src/worker.cpp
)

add_library(Cadence_lib
    STATIC
    ${CADENCE_SRC}
)

target_link_libraries(Cadence_lib
    PUBLIC 
    Threads::Threads
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


include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(Cadence_tests
    tests/construction_test.cpp
)


target_link_libraries(Cadence_tests
    Cadence_lib
    GTest::gtest_main
)

add_test(NAME Cadence_tests COMMAND Cadence_tests)