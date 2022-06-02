cmake_minimum_required(VERSION 3.15)
project(libnum VERSION 0.1)

# Most of the code is in the headers
add_library(num STATIC src/static_checks.cpp test.cpp)

target_include_directories(num
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(num PRIVATE -std=c++17)

#---
# Install
#---

# Library file: libnum.a
install(TARGETS num DESTINATION ${LIBDIR})
# Headers: azur/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR})
