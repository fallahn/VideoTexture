cmake_minimum_required(VERSION 2.8.12)

project(VideoTexture)
SET(PROJECT_NAME vtex)

if(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
SET(PROJECT_STATIC_RUNTIME FALSE CACHE BOOL "Use statically linked standard/runtime libraries?")
#SET(PROJECT_STATIC_TMX FALSE CACHE BOOL "Use statically linked tmxlite library?")

if(CMAKE_COMPILER_IS_GNUCXX OR APPLE)
  if(PROJECT_STATIC_RUNTIME)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -static")
  else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14")
  endif()
endif()

SET (CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG_")
SET (CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
SET (OpenGL_GL_PREFERENCE "GLVND")

find_package(SFML 2 REQUIRED graphics window system audio)
find_package(OpenGL REQUIRED)

include_directories(
  ${SFML_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR})

set(PROJECT_SRC VideoTexture/main.cpp VideoTexture/src/VideoTexture.cpp)

if(WIN32)
  add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SRC})
else()
  add_executable(${PROJECT_NAME} ${PROJECT_SRC})
endif()

target_link_libraries(${PROJECT_NAME}
  ${SFML_LIBRARIES}
  ${OPENGL_LIBRARIES})

#install executable
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION .)

#install game data
install(DIRECTORY assets
  DESTINATION .)
