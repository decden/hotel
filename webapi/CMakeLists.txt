add_library(hotel_webapi
            server.cpp
            )

target_link_libraries(hotel_webapi hotel mongoose)
