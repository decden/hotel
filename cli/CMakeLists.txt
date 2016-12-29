set(SRC
    cli.cpp
    testdata.cpp
)

set(SRC_INCLUDES
    testdata.h
)

add_executable(hotel_cli  ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel_cli hotel)
