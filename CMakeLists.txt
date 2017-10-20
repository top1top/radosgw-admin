cmake_minimum_required(VERSION 3.8)
project(radosgw_admin)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost 1.65.1 COMPONENTS program_options REQUIRED)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(SOURCE_FILES main.cpp)
    add_executable(radosgw_admin ${SOURCE_FILES})
    target_link_libraries(radosgw_admin ${Boost_LIBRARIES})
endif()