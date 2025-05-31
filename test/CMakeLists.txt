include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/bernlohr)
add_executable(test_simtel_filehandler test_simtel_filehandler.cpp )
target_link_libraries(test_simtel_filehandler PRIVATE simtel_event)

add_executable(test_simtel_eventsource test_simteleventsource.cpp)
target_link_libraries(test_simtel_eventsource PRIVATE simtel_event)

add_executable(test_camera_geometry test_camera_geometry.cpp)
target_link_libraries(test_camera_geometry PRIVATE basic_event)
add_test(NAME test_simtel_filehandler
    COMMAND test_simtel_filehandler
    )
add_test(NAME test_simtel_eventsource
    COMMAND test_simtel_eventsource
    )
add_test(NAME test_camera_geometry
    COMMAND test_camera_geometry
    )