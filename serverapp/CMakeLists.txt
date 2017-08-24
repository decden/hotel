set(SRC
    serverapp.cpp
)

set(SRC_INCLUDES
)

add_executable(hotel_serverapp ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel_serverapp persistence hotel hotel_gui hotel_server)
