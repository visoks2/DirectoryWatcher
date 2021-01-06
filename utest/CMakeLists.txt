cmake_minimum_required (VERSION 3.12)
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)
include(GoogleTest)

add_executable(tests main.cpp)
target_link_libraries(tests 
    ${GTEST_LIBRARIES} 
    ${GMOCK_LIBRARIES} 
    pthread
    )
target_link_libraries(tests 
    # zzzzzzzz
)

gtest_discover_tests(tests)
