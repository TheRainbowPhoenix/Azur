#-----------------------------------------------------------------------------#
#  ,"  /\  ",    Azur: A game engine for CASIO fx-CG and PC                   #
# |  _/__\_  |   Designed by Lephe' and the Planète Casio community.          #
#  "._`\/'_."    License: MIT <https://opensource.org/licenses/MIT>           #
#-----------------------------------------------------------------------------#
# libnum build system

add_library(num STATIC
  src/static_checks.cpp)

target_include_directories(num PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Library file: libnum.a
install(TARGETS num DESTINATION ${LIBDIR})
# Headers: azur/*.h
install(DIRECTORY include/ DESTINATION ${INCDIR})
