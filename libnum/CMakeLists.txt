#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# libnum build system

include(CTest)

set(SOURCES
  src/num.cpp
  src/str.cpp)

if(DEFINED AZUR_ARCH_GENERIC)
  set(LIBNUM_ARCH_GENERIC TRUE)
elseif(DEFINED AZUR_ARCH_SH4ALDSP)
  set(LIBNUM_ARCH_SH4ALDSP TRUE)
  list(APPEND SOURCES
    src/sh4aldsp/div_i32_i16_i16.s)
endif()

configure_file(include/num/config.h.in num/config.h)

add_library(num STATIC ${SOURCES})
target_include_directories(num PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_BINARY_DIR}")

# Library file: libnum.a
install(TARGETS num DESTINATION ${LIBDIR})
# Headers: num/config.h and num/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR}/libnum)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/num/config.h"
        DESTINATION ${INCDIR}/libnum/num)

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
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test/isel.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/${testfile}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include")
endforeach()
