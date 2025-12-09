cmake_minimum_required(VERSION 3.20)

project(A)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(LMtest LMtest.cpp LMAlgorithm.hpp HelperFunction.hpp)
target_compile_options(LMtest PUBLIC -g3 -Wall -Wpedantic -Wextra -fno-exceptions -fno-rtti)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        cd430b47a54841ec45d64d2377d7cabaf0eba610)
FetchContent_MakeAvailable(googletest)

target_link_libraries(LMtest PUBLIC gtest)