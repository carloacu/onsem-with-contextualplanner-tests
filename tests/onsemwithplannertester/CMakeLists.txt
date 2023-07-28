project(onsemwithplannertester)
cmake_minimum_required(VERSION 2.8)



# Making a lib
set(ONSEMWITHPLANNERTESTER_HPPS
  include/onsem/optester/api.hpp
  include/onsem/optester/loadchatbot.hpp
)

set(ONSEMWITHPLANNERTESTER_SRCS
  src/loadchatbot.cpp
)

include_directories(include)


if(NOT ONSEMCOMMON_FOUND)
  find_package(onsemcommon)
endif()
if(NOT ONSEMTEXTTOSEMANTIC_FOUND)
  find_package(onsemtexttosemantic)
endif()
if(NOT ONSEMSEMANTICTOTEXT_FOUND)
  find_package(onsemsemantictotext)
endif()
if(NOT ONSEMSEMANTOCDEBUGGER_FOUND)
  find_package(onsemsemanticdebugger)
endif()
if(NOT CONTEXTUALPLANNER_FOUND)
  find_package(contextualplanner)
endif()

add_library(onsemwithplannertester SHARED
  ${ONSEMWITHPLANNERTESTER_HPPS}
  ${ONSEMWITHPLANNERTESTER_SRCS}
)

include_directories(
  ${BOOST_PROPERTY_TREE_WITH_DEPS_INCLUDE_DIRS}
  ${ONSEMCOMMON_INCLUDE_DIRS}
  ${ONSEMTEXTTOSEMANTIC_INCLUDE_DIRS}
  ${ONSEMSEMANTICTOTEXT_INCLUDE_DIRS}
  ${ONSEMSEMANTOCDEBUGGER_INCLUDE_DIRS}
  ${CONTEXTUALPLANNER_INCLUDE_DIRS}
)

target_link_libraries(onsemwithplannertester
  ${ONSEMCOMMON_LIBRARIES}
  ${ONSEMTEXTTOSEMANTIC_LIBRARIES}
  ${ONSEMSEMANTICTOTEXT_LIBRARIES}
  ${ONSEMSEMANTOCDEBUGGER_LIBRARIES}
  ${CONTEXTUALPLANNER_LIBRARIES}
)
target_compile_features(onsemwithplannertester PRIVATE cxx_std_17)

