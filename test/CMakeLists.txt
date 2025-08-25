include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/bernlohr)
add_executable(test_simtel_filehandler test_simtel_filehandler.cpp )
target_link_libraries(test_simtel_filehandler PRIVATE simtel_event)

add_executable(test_simtel_eventsource test_simteleventsource.cpp)
target_link_libraries(test_simtel_eventsource PRIVATE simtel_event)

add_executable(test_camera_geometry test_camera_geometry.cpp)
target_link_libraries(test_camera_geometry PRIVATE basic_event)

add_executable(test_muparser_query test_muparser_query.cpp)
target_link_libraries(test_muparser_query query)

add_executable(test_data_writer test_data_writer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../root/RootWriter.cpp)
target_link_libraries(test_data_writer PUBLIC root_writer root_event simtel_event)

add_test(NAME test_simtel_filehandler
    COMMAND test_simtel_filehandler
    )
add_test(NAME test_simtel_eventsource
    COMMAND test_simtel_eventsource
    )
add_test(NAME test_camera_geometry
    COMMAND test_camera_geometry
    )
add_test(NAME test_muparser_query
    COMMAND test_muparser_query)
add_test(NAME test_data_writer
    COMMAND test_data_writer)