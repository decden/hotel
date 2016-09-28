set(CMAKE_AUTOMOC ON)

add_library(hotel_gui
            statusbar.cpp
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

#
# Link dependencies
#

target_link_libraries(hotel_gui hotel Qt5::Widgets)
