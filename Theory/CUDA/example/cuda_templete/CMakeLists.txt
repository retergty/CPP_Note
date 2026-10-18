cmake_minimum_required(VERSION 3.18)
project(cuda_practice LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES 75)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options("$<$<COMPILE_LANGUAGE:CUDA>:-G>")
endif()

# clangd 不能直接消化 nvcc 的 compile_commands，另写一份给语言服务用。
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/gen_clangd_cdb.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()

set(HELLO_MAIN "${CMAKE_SOURCE_DIR}/src/hello.cu")
if(NOT EXISTS "${HELLO_MAIN}")
  message(FATAL_ERROR "src/hello.cu is required (the only program entry)")
endif()

file(GLOB CUDA_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cu")
add_executable(hello ${CUDA_SOURCES})
target_include_directories(hello PRIVATE "${CMAKE_SOURCE_DIR}/include")
set_target_properties(hello PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  CUDA_SEPARABLE_COMPILATION ON
)
