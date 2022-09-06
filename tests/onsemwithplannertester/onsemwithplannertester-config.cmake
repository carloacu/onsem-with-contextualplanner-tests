get_filename_component(_onsemwithplannertester_root "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_onsemwithplannertester_root "${_onsemwithplannertester_root}" ABSOLUTE)


set(ONSEMWITHPLANNERTESTER_FOUND TRUE)

set(
  ONSEMWITHPLANNERTESTER_INCLUDE_DIRS
  ${_onsemwithplannertester_root}/include
  CACHE INTERNAL "" FORCE
)

set(
  ONSEMWITHPLANNERTESTER_LIBRARIES
  "onsemwithplannertester"
)

