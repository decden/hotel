cmake_minimum_required (VERSION 2.8.11)
project (hotel)
set (CMAKE_CXX_STANDARD 14)
include_directories(.)

#
# Libraries
#

add_subdirectory (cli)
add_subdirectory (gui)
add_subdirectory (hotel)

#
# Applications
#

add_subdirectory (guiapp)