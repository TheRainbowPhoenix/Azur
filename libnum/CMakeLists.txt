#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# libnum build system

include(CTest)

add_library(num STATIC
  src/str.cpp)

target_include_directories(num PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Library file: libnum.a
install(TARGETS num DESTINATION ${LIBDIR})
# Headers: azur/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR})

#---
# Unit tests
#---

set(UNIT_TESTS
  test/unit_scalar.cpp
  test/unit_static.cpp)

if(NOT CMAKE_CROSSCOMPILING)
  add_executable(numtest ${UNIT_TESTS})
  target_link_libraries(numtest PUBLIC num)

  add_test(NAME "UnitTests" COMMAND numtest)
endif()

#---
# Performance tests
#---

set(PERF_TESTS
  test/isel_num8.cpp
  test/isel_num16.cpp
  test/isel_num32.cpp)

foreach(testfile IN LISTS PERF_TESTS)
  add_test(NAME "${testfile}"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/test/isel.py"
                   "${CMAKE_CURRENT_SOURCE_DIR}/${testfile}"
                   -I "${CMAKE_CURRENT_SOURCE_DIR}/include")
endforeach()
