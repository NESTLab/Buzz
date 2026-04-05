#
# Test programs
#
enable_testing()

add_executable(test_layer1 test_layer1.c ../buzz/buzzutils.c)
target_link_libraries(test_layer1 buzz)
add_test(NAME layer1 COMMAND test_layer1)

add_executable(test_layer2 test_layer2.c
  ../buzz/buzzutils.c
  ../buzz/buzzgsl.c)
target_link_libraries(test_layer2 buzz GSL::gsl GSL::gslcblas)
add_test(NAME layer2 COMMAND test_layer2)

add_executable(test_layer3_harness test_layer3_harness.c
  ../buzz/buzzutils.c
  ../buzz/buzzgsl.c
  ../buzz/buzzlinalg.c
  ../buzz/buzzdebug.c
  ../buzz/buzzasm.c)
target_link_libraries(test_layer3_harness buzz GSL::gsl GSL::gslcblas)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bo
  ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bdb
  COMMAND ${CMAKE_BINARY_DIR}/utility/bzzc
  -b ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bo
  -d ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bdb
  ${CMAKE_CURRENT_SOURCE_DIR}/test_layer3.bzz
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_layer3.bzz
  ${CMAKE_BINARY_DIR}/utility/bzzc
)

add_custom_target(test_layer3_bzz ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bo)

add_test(NAME layer3
  COMMAND test_layer3_harness
  ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bo
  ${CMAKE_CURRENT_BINARY_DIR}/test_layer3.bdb)

add_executable(testbuzzdarray testbuzzdarray.c)
target_link_libraries(testbuzzdarray buzz)

add_executable(testbuzzdict testbuzzdict.c)
target_link_libraries(testbuzzdict buzz)

add_executable(testbuzzset testbuzzset.c)
target_link_libraries(testbuzzset buzz)

add_executable(testbuzzlex ../buzz/buzzlex.h ../buzz/buzzlex.c testbuzzlex.c)
target_link_libraries(testbuzzlex buzz)

add_executable(testbuzzstrman testbuzzstrman.c)
target_link_libraries(testbuzzstrman buzz)

add_executable(testcallclosure testcallclosure.c)
target_link_libraries(testcallclosure buzz)

if(ARGOS_FOUND)
  if(ARGOS_BUILD_FOR STREQUAL "simulator")
    include_directories(${ARGOS_INCLUDE_DIRS})
    link_directories(${ARGOS_LIBRARY_DIR})
    link_libraries(${ARGOS_LDFLAGS})
    add_library(testloopfunctions MODULE testloopfunctions.h testloopfunctions.cpp)
    target_link_libraries(testloopfunctions argos3plugin_${ARGOS_BUILD_FOR}_buzz buzz argos3core_${ARGOS_BUILD_FOR})
  endif(ARGOS_BUILD_FOR STREQUAL "simulator")
endif(ARGOS_FOUND)

#
# Test scripts
#

function(_buzz_make_test _script)
  if(ARGN)
    if(NOT ARGV1 STREQUAL "INCLUDES")
      message(FATAL_ERROR "Expected INCLUDES in _buzz_make_test(script INCLUDES ...), got ${ARGV1}")
    endif(NOT ARGV1 STREQUAL "INCLUDES")
    if(ARGC LESS 3)
      message(FATAL_ERROR "Expected scripts to include in _buzz_make_test(script INCLUDES ...)")
    endif(ARGC LESS 3)
    buzz_make(${_script} ${ARGN})
  else(ARGN)
    buzz_make(${_script})
  endif(ARGN)
  add_dependencies(${_script} bzzasm bzzdeasm bzzparse)
endfunction(_buzz_make_test)

if(NOT CMAKE_CROSSCOMPILING)
  # Make sure only the locally compiled tools are used
  set(BUZZ_COMPILER ${CMAKE_BINARY_DIR}/utility/bzzc)
  set(BUZZ_PARSER ${CMAKE_BINARY_DIR}/buzz/bzzparse)
  set(BUZZ_ASSEMBLER ${CMAKE_BINARY_DIR}/buzz/bzzasm)
  set(BUZZ_BZZ_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include)
  # Initialize Buzz CMake stuff
  set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/utility)
  include(UseBuzz)

  _buzz_make_test(test_takeoff_divide_flock.bzz)
  _buzz_make_test(testempty.bzz)
  _buzz_make_test(testif.bzz)
  _buzz_make_test(testdebug.bzz)
  _buzz_make_test(testdocstring.bzz)
  _buzz_make_test(testbehavior.bzz)
  _buzz_make_test(testclosure.bzz)
  _buzz_make_test(testhexagon.bzz)
  _buzz_make_test(testsquare.bzz)
  _buzz_make_test(testgradient.bzz)
  _buzz_make_test(testinclude1.bzz INCLUDES testinclude2.bzz)
  _buzz_make_test(testio.bzz)
  _buzz_make_test(testleds.bzz)
  _buzz_make_test(testmath.bzz)
  _buzz_make_test(testexpressions.bzz)
  _buzz_make_test(testmsg.bzz)
  _buzz_make_test(testneighbors.bzz)
  _buzz_make_test(testparsing.bzz)
  _buzz_make_test(teststigmergy.bzz)
  _buzz_make_test(teststring.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/string.bzz)
  _buzz_make_test(testswarm.bzz)
  _buzz_make_test(testtable.bzz)
  _buzz_make_test(testvec2.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/vec2.bzz)
  _buzz_make_test(testwhile.bzz)
  _buzz_make_test(testfor.bzz)
  _buzz_make_test(testmobilecode.bzz)
  _buzz_make_test(testbattery.bzz)
  _buzz_make_test(testtablelib.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/table.bzz)
  _buzz_make_test(testneighborsmapreduce.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/neighbors.bzz ${CMAKE_SOURCE_DIR}/include/table.bzz)
  _buzz_make_test(testtype.bzz)
  _buzz_make_test(testmatrix.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/matrix.bzz)
  _buzz_make_test(testqueue.bzz INCLUDES ${CMAKE_SOURCE_DIR}/include/string.bzz ${CMAKE_SOURCE_DIR}/include/table.bzz)
endif(NOT CMAKE_CROSSCOMPILING)
