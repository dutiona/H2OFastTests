cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# Standards C++11 requis.
set_property(GLOBAL PROPERTY CXX_STANDARD 11)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)

project (H2OFastTests)

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

set(
	source_files
	inc/H2OFastTests.hpp
    inc/H2OFastTests_config.hpp
)

source_group(
	"Headers"
	FILES
	inc/H2OFastTests.hpp
    inc/H2OFastTests_config.hpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_library(H2OFastTests STATIC ${source_files})
set_target_properties(H2OFastTests PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(tests)

# add the install targets
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/inc/H2OFastTests.hpp
         DESTINATION H2OFastTests)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/inc/H2OFastTests_config.hpp
         DESTINATION H2OFastTests)