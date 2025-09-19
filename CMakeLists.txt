# Paibot Drawing Tool for Geometry Dash
# Minimum versions for stability and compatibility
cmake_minimum_required(VERSION 3.21)

# Require specific Geode SDK version (4.8.0) to prevent upstream breakage
set(GEODE_REQUIRED_VERSION "4.8.0" CACHE STRING "Required Geode SDK version")

project(PaibotGeodeBase VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recommended toolchain: MSVC on Windows, Clang on other platforms
# Compilation flags for optimization and debugging
if(MSVC)
    # MSVC specific flags
    add_compile_options(/W4 /WX /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # Clang/GCC flags
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

option(GEODE_FETCH_SDK "Automatically download the Geode SDK" ON)
set(GEODE_SDK_DIR "" CACHE PATH "Path to a local clone of the Geode SDK")

if (NOT GEODE_SDK_DIR)
    set(GEODE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.geode-sdk")
endif()

if (GEODE_FETCH_SDK AND NOT EXISTS "${GEODE_SDK_DIR}/CMakeLists.txt")
    include(FetchContent)
    message(STATUS "Fetching Geode SDK ${GEODE_REQUIRED_VERSION} into ${GEODE_SDK_DIR}")
    FetchContent_Declare(geode
        GIT_REPOSITORY https://github.com/geode-sdk/geode.git
        GIT_TAG "v${GEODE_REQUIRED_VERSION}"
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
    src/manager/ToolManager.cpp
    
    # Utility classes - drawing tools
    src/util/BrushDrawer.cpp
    src/util/LineBrushDrawer.cpp
    src/util/GradientBrushDrawer.cpp
    src/util/StructureOptimizer.cpp
    src/util/BackgroundGenerator.cpp
    src/util/IntegrityLogger.cpp
    
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
