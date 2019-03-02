# Library providing futures and stream support

set(SRC
  fas.cpp
)

set(SRC_INCLUDES
  fas.h
  executorptr.h
  future.h
  queueexecutor.h
  threadedexecutor.h
  systemexecutor.h
)

add_library(fas ${SRC} ${SRC_INCLUDES})

if (test)
  add_subdirectory(tests)
endif()
