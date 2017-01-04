set(SRC
	datasource.cpp

	op/operations.cpp
)

set(SRC_INCLUDES
	datasource.h

	op/operations.h
)

add_library(persistence ${SRC} ${SRC_INCLUDES})
target_link_libraries(persistence hotel ${Boost_DATE_TIME_LIBRARY} ${SQLITE3_LIBRARY})
