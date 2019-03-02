set(SRC
  backend.cpp
  changequeue.cpp
  datastream.cpp
  datastreamobserver.cpp

  op/operations.cpp

  json/jsonserializer.cpp

  sqlite/sqlitebackend.cpp
  sqlite/sqlitestatement.cpp
  sqlite/sqlitestorage.cpp

  net/jsonserializer.cpp
  net/netclientbackend.cpp
)

set(SRC_INCLUDES
  backend.h
  changequeue.h
  datastream.h
  datastreamobserver.h
  taskresult.h

  op/operations.h

  json/jsonserializer.h

  sqlite/sqlitebackend.h
  sqlite/sqlitestatement.h
  sqlite/sqlitestorage.h

  net/jsonserializer.h
  net/netclientbackend.h
)

add_library(persistence ${SRC} ${SRC_INCLUDES})
target_link_libraries(persistence hotel)
target_link_libraries(persistence fas)
target_link_libraries(persistence ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${SQLITE3_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
