include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/bernlohr)
add_executable(test_simtel_io test_simtel_filehandler.cpp )
target_link_libraries(test_simtel_io PRIVATE simtel_event)

add_executable(test_camera_geometry test_camera_geometry.cpp )
target_link_libraries(test_camera_geometry PRIVATE  basic_event)
add_executable(test_calibration test_calibration.cpp )
target_link_libraries(test_calibration PRIVATE  basic_event calibrator)
add_executable(test_image_extractor test_image_extractor.cpp )
target_link_libraries(test_image_extractor PRIVATE  basic_event calibrator)
add_executable(test_image_processor test_image_processor.cpp )
target_link_libraries(test_image_processor PRIVATE  basic_event image_processor)
add_executable(test_muparser_query test_muparser_query.cpp )
target_link_libraries(test_muparser_query PRIVATE  query)
add_executable(test_coordinates test_coordinates.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/../src/Coordinates.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/CoordFrames.cpp)

add_executable(test_histogram test_histogram.cpp)

add_executable(test_statistics test_statistics.cpp)