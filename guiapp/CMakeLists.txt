set(SRC
    gui.cpp
)

set(SRC_INCLUDES
)

set(CMAKE_AUTOMOC ON)
add_executable(hotel_guiapp ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel_guiapp persistence hotel hotel_gui Qt5::Widgets)
