cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# Standards C++11 requis.
set_property(GLOBAL PROPERTY CXX_STANDARD 11)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")

project (H2OFastTests)

# The version number.
set (H2OFastTests_VERSION_MAJOR 1)
set (H2OFastTests_VERSION_MINOR 0)
 
configure_file (
  ${PROJECT_SOURCE_DIR}/config/H2OFastTests_config.h.in
  ${PROJECT_BINARY_DIR}/config/H2OFastTests_config.h
  )

# Activer dossiers dans Visual
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Niveau d'avertissements 4 sous Windows.
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-long-long -pedantic")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")

file(
	GLOB_RECURSE
	source_files
	inc/*
)

include_directories(
	inc/
)

source_group(
	"Headers"
	FILES
	inc/H2OFastTests.h
)

add_library(H2OFastTests STATIC ${source_files})
set_target_properties(H2OFastTests PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(tests)
 
 set(CONFIGURED_ONCE TRUE CACHE INTERNAL
    "A flag showing that CMake has configured at least once.")