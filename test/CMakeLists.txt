include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/bernlohr)
add_executable(test_simtel_filehandler test_simtel_filehandler.cpp )
target_link_libraries(test_simtel_filehandler PRIVATE simtel_event)

add_test(NAME test_simtel_filehandler
    COMMAND test_simtel_filehandler
    )