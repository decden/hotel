set(SRC
    test_hotel.cpp
    test_hotel_planning.cpp
)

set(SRC_INCLUDES
)

add_executable(tests ${SRC} ${SRC_INCLUDES})
target_link_libraries(tests gtest gtest_main gmock)
target_link_libraries(tests hotel ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
