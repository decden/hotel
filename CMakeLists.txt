cmake_minimum_required (VERSION 2.8.11)
project (hotel)
set (CMAKE_CXX_STANDARD 14)

include_directories (.)
include_directories(./mongoose)
add_subdirectory (cli)
add_subdirectory (hotel)
add_subdirectory (webapi)
