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

target_link_libraries(hotel sqlite3 boost_date_time)
