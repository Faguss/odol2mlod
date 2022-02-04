cmake_minimum_required(VERSION 2.8.9)
project (OdolToMlod)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_definitions(-ggdb)

add_executable(odol2mlod
	main.cpp
)

