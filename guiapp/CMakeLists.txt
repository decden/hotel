set(CMAKE_AUTOMOC ON)

add_executable(hotel_guiapp
               gui.cpp
              )

#
# Link dependencies
#

target_link_libraries(hotel_guiapp hotel hotel_gui Qt5::Widgets)