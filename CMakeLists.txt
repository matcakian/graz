cmake_minimum_required(VERSION 3.20)

project(graz)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lmsr source/lmsr.cpp)
target_include_directories(lmsr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(lmsr_tests tests/lmsr_tests.cpp)

target_link_libraries(lmsr_tests PRIVATE lmsr Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(lmsr_tests)
