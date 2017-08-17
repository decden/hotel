set(SRC
    server.cpp
)

set(SRC_INCLUDES
)

set(CMAKE_AUTOMOC ON)
add_executable(hotel_serverapp ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel_serverapp persistence hotel hotel_gui Qt5::Widgets)
