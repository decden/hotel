# Library providing futures and stream support

set(SRC
  cancellation.cpp
  fas.cpp
  future.cpp
)

set(SRC_INCLUDES
  fas.h
  executorptr.h
  future.h
  stream.h
  queueexecutor.h
  threadedexecutor.h
  systemexecutor.h
  cancellation.h
)

add_library(fas ${SRC} ${SRC_INCLUDES})

if (test)
  add_subdirectory(tests)
endif()
