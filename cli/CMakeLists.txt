add_executable(hotel_cli
               cli.cpp
               testdata.cpp
              )

target_link_libraries(hotel_cli hotel mongoose)
