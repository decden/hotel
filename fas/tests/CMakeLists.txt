set(SRC
  test_future.cpp
)

set(SRC_INCLUDES
)

add_executable(test_fas ${SRC} ${SRC_INCLUDES})
target_link_libraries(test_fas fas)
target_link_libraries(test_fas gtest gtest_main gmock)
