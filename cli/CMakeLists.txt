add_executable(hotel_cli
               cli.cpp
               testdata.cpp
              )

#
# Link dependencies
#

target_link_libraries(hotel_cli hotel)
