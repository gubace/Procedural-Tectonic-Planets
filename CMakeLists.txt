cmake_minimum_required(VERSION 3.5)
project(Projet3D VERSION 1.0 LANGUAGES CXX)

# Prefer GLVND with modern CMake
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

# Options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type default (force to Debug if not given)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Debug flags (enlever -lGLEW d'ici, ce n'est pas un flag de compilation)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O3 ")

# Source collection
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/main.cpp
    ${SRC_DIR}/*.cpp
)

# Geometric Tools (GTE) - header-only
set(GTE_DIR ${CMAKE_SOURCE_DIR}/external/GeometricTools/GTE CACHE PATH "Path to GeometricTools GTE headers")

if(EXISTS "${GTE_DIR}")
    message(STATUS "GTE headers found: ${GTE_DIR}")
else()
    message(WARNING "GTE headers not found at ${GTE_DIR}. If you added GTE, check the path or set -DGTE_DIR=path/to/GTE")
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLEW (IMPORTANT!)
find_package(GLEW REQUIRED)
if(GLEW_FOUND)
    message(STATUS "Found GLEW: ${GLEW_LIBRARIES}")
    message(STATUS "GLEW include: ${GLEW_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "GLEW not found. Install libglew-dev.")
endif()

# Try to find GLUT
find_package(GLUT)
if(NOT GLUT_FOUND)
    find_library(GLUT_LIB glut)
    if(NOT GLUT_LIB)
        message(FATAL_ERROR "GLUT not found. Install freeglut3-dev / freeglut-devel or adjust CMake.")
    else()
        message(STATUS "Found GLUT (fallback): ${GLUT_LIB}")
    endif()
else()
    message(STATUS "Found GLUT via find_package")
endif()

# Find GLU
find_library(GLU_LIB GLU)
if (GLU_LIB)
    message(STATUS "Found GLU: ${GLU_LIB}")
else()
    message(FATAL_ERROR "GLU library not found. Install libglu (e.g. libglu1-mesa-dev) or adjust CMake.")
endif()

# Create executable target
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# Include dirs
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${GTE_DIR}
    ${GLEW_INCLUDE_DIRS}  # IMPORTANT: ajouter les headers GLEW
)

# Link OpenGL
if(TARGET OpenGL::GL)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
elseif(DEFINED OPENGL_gl_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_gl_LIBRARY})
elseif(DEFINED OPENGL_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
endif()

# Link GLEW (IMPORTANT: avant GLUT!)
if(TARGET GLEW::GLEW)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLEW_LIBRARIES})
endif()

# Link GLUT
if(TARGET GLUT::GLUT)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLUT::GLUT)
elseif(DEFINED GLUT_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLUT_LIBRARY})
elseif(DEFINED GLUT_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLUT_LIB})
endif()

# link GLU
target_link_libraries(${PROJECT_NAME} PRIVATE ${GLU_LIB})

# math / threading
target_link_libraries(${PROJECT_NAME} PRIVATE m)
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)