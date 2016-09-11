set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)

add_executable(hotel_gui
               gui.cpp
               planningboardwidget.cpp
              )

target_link_libraries(hotel_gui hotel hotel_webapi Qt5::Widgets)
