cmake_minimum_required(VERSION 3.20)
project(SyntaxAnalysis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option A: glob all .cpp files in src
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(SyntaxAnalysis ${SOURCES})
target_include_directories(SyntaxAnalysis PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
