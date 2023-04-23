#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# libnum build system

include(CTest)

add_library(num STATIC
  src/num.cpp
  src/str.cpp)

target_include_directories(num PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Library file: libnum.a
install(TARGETS num DESTINATION ${LIBDIR})
# Headers: azur/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR})

#---
# Unit tests
#---

set(UNIT_TESTS_SCALAR
  test/unit_scalar.cpp
  test/unit_static.cpp)

set(UNIT_TESTS_STR
  test/unit_str.cpp)

if(NOT CMAKE_CROSSCOMPILING)
  add_executable(numtest_scalar ${UNIT_TESTS_SCALAR})
  target_link_libraries(numtest_scalar PUBLIC num)
  add_test(NAME "UnitTestsScalar" COMMAND numtest_scalar)
endif()

if(NOT CMAKE_CROSSCOMPILING)
  add_executable(numtest_str ${UNIT_TESTS_STR})
  target_link_libraries(numtest_str PUBLIC num)
  add_test(NAME "UnitTestsStr" COMMAND numtest_str)
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
