cmake_minimum_required( VERSION 3.2 )

if( NOT WIN32 )
  set( CMAKE_CXX_COMPILER "/usr/bin/clang++" )
  set( CMAKE_C_COMPILER   "/usr/bin/clang"   )
endif( NOT WIN32 )

set( CMAKE_CXX_STANDARD 17          )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

set( PROJECT_NAME Vulkan )
project( ${PROJECT_NAME} )

# Find vulkan
find_package( Vulkan 1.0 REQUIRED )

# Find GLFW
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE )
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE )
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

find_package( glfw3 REQUIRED )

# Set source files
set( SOURCES
    "main.cpp"
    "vulkan.cpp"
    "vulkan.h"
    "utils.cpp"
    "utils.h"
    "vertices.cpp"
    "vertices.h"
    "submodules/stb-lib/stb_image.h"
    "submodules/tiny_obj_loader/tiny_obj_loader.h" )

add_executable( ${PROJECT_NAME} ${SOURCES} )

if ( WIN32 )
  target_compile_definitions( ${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif( WIN32 )

if ( UNIX AND NOT APPLE )
  find_package   ( X11 REQUIRED               )
  link_libraries ( ${X11_LIBRARIES}           )
  add_definitions( -DVK_USE_PLATFORM_XLIB_KHR )
endif ( UNIX AND NOT APPLE )

target_link_libraries( ${PROJECT_NAME} Vulkan::Vulkan glfw )

# Compile shaders before building
if ( WIN32 )
  message( INFO "Please compile your shaders manually" )
endif( WIN32 )

if( NOT WIN32 )
  if( EXISTS "${CMAKE_CURRENT_LIST_DIR}/ressources/SPIR_V_COMPILE.sh" )
    add_custom_command( TARGET ${PROJECT_NAME} PRE_BUILD COMMAND /usr/local/bin/zsh ${CMAKE_CURRENT_LIST_DIR}/ressources/SPIR_V_COMPILE.sh ${CMAKE_CURRENT_LIST_DIR} )
  else()
    message( WARNING "SPIR_V_COMPILE.bat not found. Please manual compile your shaders" )
  endif()
endif( NOT WIN32 )

# Copy files to binary directory
file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/ressources/triangle.frag.spv DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/ressources/triangle.vert.spv DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/models/chalet.mdl DESTINATION ${CMAKE_CURRENT_BINARY_DIR}            )
file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures/chalet.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR}          )