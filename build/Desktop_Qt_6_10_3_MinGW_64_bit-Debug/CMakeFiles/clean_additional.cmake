# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\TimeTracker_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\TimeTracker_autogen.dir\\ParseCache.txt"
  "TimeTracker_autogen"
  )
endif()
