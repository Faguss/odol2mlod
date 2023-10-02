cmake_minimum_required(VERSION 3.27)
project (OdolToMlod)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_definitions(-ggdb)

add_executable(odol2mlod
	main.cpp
)

