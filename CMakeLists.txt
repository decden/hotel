cmake_minimum_required (VERSION 3.1)

option(test "Build test application." ON)
option(build_gui "Build gui libraries and applications." ON)

project (hotel)
set (CMAKE_CXX_STANDARD 14)
include_directories(.)

#
# Find and configure external dependencies
#

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
endif()

find_package (Threads)

find_package(Boost 1.58.0 REQUIRED COMPONENTS date_time system)
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

include_directories(./extern/nlohmann_json)

if (build_gui)
  find_package(Qt5Widgets REQUIRED)
endif()

#
# Tests
#

if (test)
	add_subdirectory(extern/googletest-release-1.8.0)
	enable_testing()
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
	include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

	add_subdirectory(tests)
endif()

#
# Subdirectories
#

add_subdirectory(hotel)
add_subdirectory(persistence)
add_subdirectory(server)

if (build_gui)
  add_subdirectory(gui)
  add_subdirectory(guiapp)
  add_subdirectory(serverapp)
endif()
