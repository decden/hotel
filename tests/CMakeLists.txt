set(SRC
    test_hotel.cpp
)

set(SRC_INCLUDES
    test_hotel.h
)

add_executable(tests ${SRC} ${SRC_INCLUDES})
target_link_libraries(tests gtest gtest_main)
target_link_libraries(tests hotel ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
