cmake_minimum_required(VERSION 2.8)
project(onsem-with-contextualplanner-tests)


option(
  USE_BOOST_FROM_SYSTEM
  "Build with the dependency of the boost installed in the system."
  ON)

option(
  BUILD_ONSEM_CHATBOTPLANNER
  "Build the chatbot planner."
  ON)




## Command line for database compilation
# cmake -DBUILD_ONSEM_DATABASE=ON -DCMAKE_BUILD_TYPE=Debug ../ && make -j4

## Command line for compilation
# cmake  -DBUILD_ONSEM_DATABASE=OFF -DCMAKE_BUILD_TYPE=Debug -DBUILD_ONSEM_TESTS=ON ../ && make -j4


include(onsem-with-contextualplanner-tests-config.cmake)
add_subdirectory(onsem)

if (NOT BUILD_ONSEM_DATABASE)
  if (BUILD_ONSEM_CHATBOTPLANNER OR BUILD_ONSEM_TESTS)
    add_subdirectory(contextualplanner)
  endif()

  if (BUILD_ONSEM_TESTS)
    add_subdirectory(tests)
    add_subdirectory(voicebotgui)
  endif()
endif()
