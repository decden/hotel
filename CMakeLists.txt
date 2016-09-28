cmake_minimum_required (VERSION 2.8.11)
project (hotel)
set (CMAKE_CXX_STANDARD 14)
include_directories(.)

#
# Find and configure external dependencies
#

find_package(Boost REQUIRED COMPONENTS date_time)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

include_directories(./extern/nlohmann_json)

find_package(Qt5Widgets REQUIRED)

#
# Libraries
#

add_subdirectory (hotel)
add_subdirectory (cli)
add_subdirectory (gui)

#
# Applications
#

add_subdirectory (guiapp)