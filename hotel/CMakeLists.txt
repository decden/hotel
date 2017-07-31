set(SRC
    hotel.cpp
    hotelcollection.cpp
    persistentobject.cpp
    person.cpp
    planning.cpp
    reservation.cpp
)

set(SRC_INCLUDES
    hotel.h
    hotelcollection.h
    persistentobject.h
    person.h
    planning.h
    reservation.h
)

add_library(hotel ${SRC} ${SRC_INCLUDES})
target_link_libraries(hotel ${Boost_DATE_TIME_LIBRARY})
