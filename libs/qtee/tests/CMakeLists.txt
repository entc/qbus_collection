SET (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
find_package(CPPUTEST)
#find_package(CPPUTEST_EXT)

message(STATUS "Found CppUTest includes ${CPPUTEST_INCLUDE_DIRS}")
message(STATUS "Found CppUTest libraries ${CPPUTEST_LIBRARIES}")
#message(STATUS "Found CppUTest_Ext includes ${CPPUTEST_EXT_INCLUDE_DIRS}")
#message(STATUS "Found CppUTest_Ext libraries ${CPPUTEST_EXT_LIBRARIES}")

if(CPPUTEST_FOUND)

# abstract operation-system layer
INCLUDE_DIRECTORIES(${CPPUTEST_INCLUDE_DIRS} "../../cape/src" "../src")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (qtee_ut_template qtee_ut_template.c)
target_link_libraries   (qtee_ut_template qtee cape ${CPPUTEST_LIBRARIES})

add_executable          (qtee_ut_eval qtee_ut_eval.cpp)
target_link_libraries   (qtee_ut_eval qtee cape ${CPPUTEST_LIBRARIES})

endif()
