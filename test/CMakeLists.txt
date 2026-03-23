include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/bernlohr)
add_executable(test_simtel_filehandler test_simtel_filehandler.cpp )
target_link_libraries(test_simtel_filehandler PRIVATE simtel_event)

add_executable(test_simtel_eventsource test_simteleventsource.cpp)
target_link_libraries(test_simtel_eventsource PRIVATE simtel_event)

add_executable(test_camera_geometry test_camera_geometry.cpp)
target_link_libraries(test_camera_geometry PRIVATE basic_event)

add_executable(test_image_processor test_image_processor.cpp)
target_link_libraries(test_image_processor PRIVATE image_processor)
add_executable(test_muparser_query test_muparser_query.cpp)
target_link_libraries(test_muparser_query query)

add_executable(test_data_writer test_data_writer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../root/RootWriter.cpp)
target_link_libraries(test_data_writer PUBLIC root_writer root_event simtel_event)

add_executable(test_configurable test_configurable.cpp )
target_include_directories(test_configurable PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include/external)

add_executable(test_interpolation test_interpolation.cpp)
target_include_directories(test_interpolation PUBLIC ${ROOT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../root/include)
target_link_libraries(test_interpolation PUBLIC root_event)

add_executable(test_coordinates test_coordinates.cpp)
target_link_libraries(test_coordinates PRIVATE reconstructor)

add_executable(test_build_index test_build_index.cpp)
target_link_libraries(test_build_index PUBLIC root_event)


#add_executable(test_lgbm_loader test_LgbmLoader.cpp)
#target_link_libraries(test_lgbm_loader PRIVATE reconstructor_ext)
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
add_test(NAME test_configurable
    COMMAND test_configurable)
add_test(NAME test_image_processor
    COMMAND test_image_processor)
add_test(NAME test_interpolation
    COMMAND test_interpolation)
add_test(NAME test_coordinates
    COMMAND test_coordinates)
add_test(NAME test_build_index
    COMMAND test_build_index)
#add_test(NAME test_lgbm_loader
#    COMMAND test_lgbm_loader)