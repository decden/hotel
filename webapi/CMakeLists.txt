add_library(mongoose
            ../mongoose/mongoose.c)

add_library(hotel_webapi
            server.cpp
            )

add_definitions(-DMG_ENABLE_SSL)
target_link_libraries(hotel_webapi hotel mongoose ssl crypto)
