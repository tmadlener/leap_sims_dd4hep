cmake_minimum_required(VERSION 3.25)

project(leap_sims_dd4hep LANGUAGES CXX)

set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)
set(${PROJECT_NAME}_VERSION_PATCH 99)

set(${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

include(cmake/Key4hepConfig.cmake)

find_package(DD4hep REQUIRED COMPONENTS DDG4)
# brings in dd4hep_add_plugin and dd4hep_generate_rootmap
include(${DD4hep_DIR}/cmake/DD4hep.cmake)

# dd4hep_generate_rootmap runs listcomponents in ${LIBRARY_OUTPUT_PATH}
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

dd4hep_instantiate_package(LEAP)
if(EXISTS "${PROJECT_BINARY_DIR}/thisLEAP.sh")
  message(STATUS "thisLEAP.sh created successfully.")
else()
  message(WARNING "thisLEAP.sh was not created.")
endif()

add_subdirectory(src)

