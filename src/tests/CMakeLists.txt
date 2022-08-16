link_libraries(acdb_s)

set(TEST_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(TESTS test1)

foreach(TN IN ITEMS ${TESTS})
  add_executable(${TN} ${TN}.c)
  set_target_properties(${TN} PROPERTIES COMPILE_FLAGS "-DIW_STATIC -DIW_TESTS")
  add_test(
    NAME ${TN}
    WORKING_DIRECTORY ${TEST_DATA_DIR}
    COMMAND ${TEST_TOOL_CMD} $<TARGET_FILE:${TN}>)
endforeach()
