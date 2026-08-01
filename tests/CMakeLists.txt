message("-- Processing: ${CMAKE_CURRENT_LIST_FILE}")

# Gather all the source test files
file(GLOB PLITY_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

if(NOT PLILY_TEST_SOURCES)
  message(WARNING "No tests found under ${CMAKE_CURRENT_SOURCE_DIR}")
endif()


# Loop through each test file and generate an executable for each one
foreach(test_file ${PLILY_TEST_SOURCES})
  # Extract filename without path and extension
  get_filename_component(file_name ${test_file} NAME_WE)

  # Create executable
  add_executable(${file_name} ${test_file})

  # Link the executable with our library
  target_link_libraries(${file_name} PRIVATE plily cunit)

  # Register the executable with CTest execution engine
  add_test(NAME ${file_name} COMMAND ${file_name})
endforeach()
