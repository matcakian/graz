cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(Graz VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(market source/market.cpp)
target_include_directories(market PUBLIC ${CMAKE_SOURCE_DIR}/include)