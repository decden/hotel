set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)

add_executable(hotel_gui
               gui.cpp
               planningboardlayout.cpp
               planningboardwidget.cpp
               planningboardreservationitem.cpp
               roomlistwidget.cpp
              )

target_link_libraries(hotel_gui hotel hotel_webapi Qt5::Widgets)
