cmake_minimum_required(VERSION 3.2)

######## Project settings ########
project(azscfgsto)
set(LICENSE "MIT")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")

######## Primary target ########
add_library(azscfgsto STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/config_store.c)

target_include_directories(azscfgsto
    PUBLIC
        inc
)

######## Install targets ########
install(TARGETS azscfgsto
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(FILES inc/config_store.h DESTINATION include)

######## Test targets ########

add_executable(azscfgsto_unittests
    tests/config_store_tests.cc
)

target_compile_features(azscfgsto_unittests PRIVATE cxx_std_17)

target_link_libraries(azscfgsto_unittests PRIVATE
    azscfgsto
    gmock
    gtest
    gtest_main
    pthread
)