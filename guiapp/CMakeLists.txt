set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)

add_executable(hotel_guiapp
               gui.cpp
              )

target_link_libraries(hotel_guiapp hotel hotel_gui)
