find_package(PkgConfig REQUIRED)
pkg_search_module(CPPUTEST REQUIRED cpputest>=3.8)

message(STATUS "Found CppUTest version ${CPPUTEST_VERSION}")
message(STATUS "Found CppUTest includes ${CPPUTEST_INCLUDE_DIRS}")
message(STATUS "Found CppUTest libraries ${CPPUTEST_LIBRARIES}")

if(CPPUTEST_FOUND)

# abstract operation-system layer
INCLUDE_DIRECTORIES(${CPPUTEST_INCLUDE_DIRS} "../../cape/src" "../src")
SET(CMAKE_BUILD_TYPE Release)                                   # build type

add_executable          (qtee_ut_template qtee_ut_template.c)
target_link_libraries   (qtee_ut_template qtee cape)

add_executable          (qtee_ut_eval qtee_ut_eval.c)
target_link_libraries   (qtee_ut_eval qtee cape)

endif()
