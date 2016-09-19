set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)

add_library(hotel_gui
            planningwidget.cpp
            planningwidget/datebarwidget.cpp
            planningwidget/planningboardlayout.cpp
            planningwidget/planningboardreservationitem.cpp
            planningwidget/planningboardwidget.cpp
            planningwidget/reservationrenderer.cpp
            planningwidget/roomlistwidget.cpp

            planningwidget/tool.cpp
            planningwidget/newreservationtool.cpp
           )

target_link_libraries(hotel_gui hotel Qt5::Widgets)
