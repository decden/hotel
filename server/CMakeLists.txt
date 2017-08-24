set(SRC
    messagesender.cpp
    netclientsession.cpp
    netserver.cpp
)

set(SRC_INCLUDES
    netclientsession.h
    messagesender.h
    netserver.h
)

add_library(hotel_server ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel_server persistence hotel)
