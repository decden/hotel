set(SRC
  datasource.cpp
  datastream.cpp
  resultintegrator.cpp

  op/operations.cpp
  op/results.cpp
  op/task.cpp

  json/jsonserializer.h

  sqlite/sqlitebackend.cpp
  sqlite/sqlitestatement.cpp
  sqlite/sqlitestorage.cpp
)

set(SRC_INCLUDES
  datasource.h
  datastream.h
  resultintegrator.h

  op/operations.h
  op/results.h
  op/task.h

  json/jsonserializer.h

  sqlite/sqlitebackend.h
  sqlite/sqlitestatement.h
  sqlite/sqlitestorage.h
)

add_library(persistence ${SRC} ${SRC_INCLUDES})
target_link_libraries(persistence hotel)
target_link_libraries(persistence ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
