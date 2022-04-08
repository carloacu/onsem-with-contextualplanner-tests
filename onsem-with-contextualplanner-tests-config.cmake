get_filename_component(_onsem_contextualplanner_tests_root "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_onsem_contextualplanner_tests_root "${onsem_contextualplanner_tests_root}" ABSOLUTE)

include(${_onsem_contextualplanner_tests_root}/contextualplanner/contextualplanner-config.cmake)
include(${_onsem_contextualplanner_tests_root}/onsem/onsem-config.cmake)
include(${_onsem_contextualplanner_tests_root}/tests/onsemtester/onsemtester-config.cmake)
