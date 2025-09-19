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

# Add all source files for the Paibot drawing tool
add_library(${PROJECT_NAME} SHARED 
    src/main.cpp
    
    # UI components (inspired by Allium)
    src/ui/PaibotButtonBar.cpp
    src/ui/MenuItemTogglerExtra.cpp
    
    # Manager classes
    src/manager/BrushManager.cpp
    
    # Utility classes - drawing tools
    src/util/BrushDrawer.cpp
    src/util/LineBrushDrawer.cpp
    src/util/GradientBrushDrawer.cpp
    src/util/StructureOptimizer.cpp
    src/util/BackgroundGenerator.cpp
    
    # TODO: Add when implemented
    # src/util/CurveBrushDrawer.cpp
    # src/util/FreeBrushDrawer.cpp
    # src/util/PolygonBrushDrawer.cpp
    # src/util/TextBrushDrawer.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include/Paibot
    third_party
)

# TODO: Add third-party libraries when ready
# CPMAddPackage(
#     GITHUB_REPOSITORY "jhasse/poly2tri"
#     GIT_TAG "13d64e7"
# )

# CPMAddPackage(
#     GITHUB_REPOSITORY "AngusJohnson/Clipper2" 
#     GIT_TAG "main"
# )

# TODO: Add Anti-Grain Geometry (AGG) library
# add_subdirectory(third_party/agg-2.6)
# target_link_libraries(${PROJECT_NAME} Agg-2.6 poly2tri Clipper2)

setup_geode_mod(${PROJECT_NAME})
