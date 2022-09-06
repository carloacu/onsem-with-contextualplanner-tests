project(onsemwithplannergtests)
cmake_minimum_required(VERSION 2.8)


set(ONSEMWITHPLANNER_GTESTS_SRCS
  src/onsemwithplannergtests.hpp
  src/test_loadchatbot.cpp
  src/main.cpp
)

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
if(NOT ONSEMSTREADATABASEACCESSOR_FOUND)
  find_package(onsemstreamdatabaseaccessor)
endif()
if(NOT CONTEXTUALPLANNER_FOUND)
  find_package(contextualplanner)
endif()
if(NOT ONSEMTESTER_FOUND)
  find_package(onsemtester)
endif()
if(NOT ONSEMWITHPLANNERTESTER_FOUND)
  find_package(onsemwithplannertester)
endif()


enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(onsemwithplannergtests
  ${ONSEMWITHPLANNER_GTESTS_SRCS}
)

include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${ONSEMCOMMON_INCLUDE_DIRS}
  ${ONSEMTEXTTOSEMANTIC_INCLUDE_DIRS}
  ${ONSEMSEMANTICTOTEXT_INCLUDE_DIRS}
  ${ONSEMSEMANTOCDEBUGGER_INCLUDE_DIRS}
  ${ONSEMSTREADATABASEACCESSOR_INCLUDE_DIRS}
  ${CONTEXTUALPLANNER_INCLUDE_DIRS}
  ${ONSEMTESTER_INCLUDE_DIRS}
  ${ONSEMWITHPLANNERTESTER_INCLUDE_DIRS}
)

target_link_libraries(onsemwithplannergtests
  ${GTEST_LIBRARIES}
  pthread
  ${ONSEMCOMMON_LIBRARIES}
  ${ONSEMTEXTTOSEMANTIC_LIBRARIES}
  ${ONSEMSEMANTICTOTEXT_LIBRARIES}
  ${ONSEMSEMANTOCDEBUGGER_LIBRARIES}
  ${ONSEMSTREADATABASEACCESSOR_LIBRARIES}
  ${CONTEXTUALPLANNER_LIBRARIES}
  ${ONSEMTESTER_LIBRARIES}
  ${ONSEMWITHPLANNERTESTER_LIBRARIES}
)
target_compile_features(onsemwithplannergtests PRIVATE cxx_std_14)


add_test(
  NAME onsemwithplannergtests
  COMMAND onsemwithplannergtests
)

