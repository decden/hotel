set(SRC
  datasource.cpp

  op/operations.cpp
  op/results.cpp

  json/jsonserializer.h

  sqlite/sqlitebackend.cpp
  sqlite/sqlitestatement.cpp
  sqlite/sqlitestorage.cpp
)

set(SRC_INCLUDES
  datasource.h

  op/operations.h
  op/results.h

  json/jsonserializer.h

  sqlite/sqlitebackend.h
  sqlite/sqlitestatement.h
  sqlite/sqlitestorage.h
)

add_library(persistence ${SRC} ${SRC_INCLUDES})
target_link_libraries(persistence hotel)
target_link_libraries(persistence ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
