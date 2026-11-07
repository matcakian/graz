cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(Graz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
include(Catch)

add_library(market source/market.cpp)
target_include_directories(market PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(market_tests test/market_tests.cpp)
target_link_libraries(market_tests PRIVATE market)
target_link_libraries(market_tests PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(market_tests)