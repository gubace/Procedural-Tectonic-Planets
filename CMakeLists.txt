cmake_minimum_required(VERSION 3.5)
project(Projet3D VERSION 1.0 LANGUAGES CXX)

# Options
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Source collection
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${SRC_DIR}/*.cpp
)

# Include directories
include_directories(${SRC_DIR})

# Find OpenGL / GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Optional: try to find GSL libs (if used)
find_library(GSL_LIB gsl)
find_library(GSLCBLAS_LIB gslcblas)

# Create executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Link libraries (portable depending on find_package results)
if(TARGET OpenGL::GL)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
elseif(DEFINED OPENGL_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
endif()

if(TARGET GLUT::GLUT)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLUT::GLUT)
elseif(DEFINED GLUT_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLUT_LIBRARY})
endif()

# math / threading
target_link_libraries(${PROJECT_NAME} PRIVATE m pthread)

# link GSL if found
if(GSL_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GSL_LIB})
endif()
if(GSLCBLAS_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GSLCBLAS_LIB})
endif()

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Installation (optional)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)