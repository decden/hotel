set(SRC
    hotel.cpp
    hotelcollection.cpp
    persistentobject.cpp
    person.cpp
    planning.cpp
    reservation.cpp

    persistence/jsonserializer.cpp
    persistence/sqlitestatement.cpp
    persistence/sqlitestorage.cpp
)

set(SRC_INCLUDES
    hotel.h
    hotelcollection.h
    persistentobject.h
    person.h
    planning.h
    reservation.h

    persistence/jsonserializer.h
    persistence/sqlitestatement.h
    persistence/sqlitestorage.h
)

add_library(hotel ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
