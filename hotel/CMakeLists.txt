add_library(hotel
            detailedreservation.cpp
            headless.cpp
            hotel.cpp
            persistentobject.cpp
            planning.cpp
            reservation.cpp

            persistence/sqlitestorage.cpp
            persistence/sqlitestatement.cpp
           )

target_link_libraries(hotel sqlite3 boost_date_time)
