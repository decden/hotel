set(CMAKE_AUTOMOC ON)

add_executable(hotel_guiapp
               gui.cpp
              )

#
# Find external dependencies
#

find_package(Qt5Widgets REQUIRED)

#
# Link dependencies
#

target_link_libraries(hotel_guiapp hotel hotel_gui Qt5::Widgets)