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

target_include_directories(hotel PUBLIC ../extern/nlohmann_json)
target_link_libraries(hotel sqlite3 boost_date_time)
