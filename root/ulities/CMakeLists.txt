add_executable(Simplified_Convert Simplified_Convert.cpp)
target_link_libraries(Simplified_Convert PUBLIC root_event reconstructor)
if(SKBUILD)
    MESSAGE(STATUS "Installing Simplified_Convert")
    install(TARGETS Simplified_Convert  DESTINATION pylast)
endif()


# Merge utility
#add_executable(merge_source merge_source.cpp)
#target_link_libraries(merge_source PUBLIC root_event reconstructor)
#if(SKBUILD)
#    MESSAGE(STATUS "Installing merge_source")
#    install(TARGETS merge_source DESTINATION pylast)
#endif()


add_executable(merge_source merge_source.cpp)
target_link_libraries(merge_source PUBLIC root_event root_writer)
if(SKBUILD)
    MESSAGE(STATUS "Installing merge_source")
    install(TARGETS merge_source DESTINATION pylast)
endif()


# Hillas reconstruction utility
add_executable(hillas_reco hillas_reco.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../RootWriter.cpp)
target_link_libraries(hillas_reco PUBLIC root_event root_writer simtel_event calibrator data_writer image_processor reconstructor)
if(SKBUILD)
    MESSAGE(STATUS "Installing hillas_reco")
    install(TARGETS hillas_reco DESTINATION pylast)
endif()

