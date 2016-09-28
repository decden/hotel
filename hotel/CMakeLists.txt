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
# Link dependencies
#

target_link_libraries(hotel ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
