cmake_minimum_required(VERSION 3.21)

project(PaibotGeodeBase VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GEODE_FETCH_SDK "Automatically download the Geode SDK" ON)
set(GEODE_SDK_DIR "" CACHE PATH "Path to a local clone of the Geode SDK")

if (NOT GEODE_SDK_DIR)
    set(GEODE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.geode-sdk")
endif()

if (GEODE_FETCH_SDK AND NOT EXISTS "${GEODE_SDK_DIR}/CMakeLists.txt")
    include(FetchContent)
    message(STATUS "Fetching Geode SDK into ${GEODE_SDK_DIR}")
    FetchContent_Declare(geode
        GIT_REPOSITORY https://github.com/geode-sdk/geode.git
        GIT_TAG main
        SOURCE_DIR "${GEODE_SDK_DIR}"
    )
    FetchContent_MakeAvailable(geode)
else()
    add_subdirectory(${GEODE_SDK_DIR} geode)
endif()

if (NOT COMMAND setup_geode_mod)
    if (EXISTS "${GEODE_SDK_DIR}/cmake/Geode.cmake")
        include(${GEODE_SDK_DIR}/cmake/Geode.cmake)
    else()
        message(FATAL_ERROR "Could not locate Geode.cmake. Please ensure the Geode SDK is available.")
    endif()
endif()

add_library(${PROJECT_NAME} SHARED src/main.cpp)

setup_geode_mod(${PROJECT_NAME})
