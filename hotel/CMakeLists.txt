add_library(hotel
            detailedreservation.cpp
            hotel.cpp
            hotelcollection.cpp
            persistentobject.cpp
            planning.cpp
            reservation.cpp

            persistence/jsonserializer.cpp
            persistence/sqlitestorage.cpp
            persistence/sqlitestatement.cpp
           )

#
# Find external dependencies
#

find_package(Boost REQUIRED COMPONENTS date_time)
link_directories(${Boost_LIBRARY_DIR})


find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
find_library(SQLITE3_LIBRARY sqlite3)

#
# Add public include path (of includes exposed by the library)
#

target_include_directories(hotel PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(hotel PUBLIC ../extern/nlohmann_json)
target_include_directories(hotel PUBLIC ${SQLITE3_INCLUDE_DIR})

#
# Link dependencies
#

target_link_libraries(hotel sqlite3 ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
