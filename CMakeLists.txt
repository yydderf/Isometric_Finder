cmake_minimum_required(VERSION 3.0)

# Project
project(Isometric_Finder)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)

include_directories(${ROOT_SOURCE_DIR}/include ${X11_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

set(SOURCES src/Isometric_Finder.cpp)

# Options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Target
add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${X11_LIBRARIES} ${OPENGL_LIBRARY} ${PNG_LIBRARY})
