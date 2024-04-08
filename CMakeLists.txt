cmake_minimum_required (VERSION 3.9)
project (mynet)
set(MYNET_VERSION_MAJOR 1)
set(MYNET_VERSION_MINOR 0)


set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -g -pthread")
include(CTest)
enable_testing()
#configure_file("${PROJECT_SOURCE_DIR}/src/mynet_config.h.in" "${PROJECT_BINARY_DIR}/mynet_config.h")

add_subdirectory(src)
add_subdirectory(demo)
#install (FILES "${PROJECT_BINARY_DIR}/mystl_config.h" DESTINATION include)
#install (FILES "${PROJECT_SOURCE_DIR}/src/base/*" DESTINATION include/base)
#install (FILES "${PROJECT_SOURCE_DIR}/src/datastruct/*" DESTINATION include/datastruct)
#add_test(arr1 ${PROJECT_SOURCE_DIR}/demo/arrdemo)
#set_tests_properties(arr1 PROPERTIES pass_regular_expression "xxx")