#add_executable(Simplified_Convert Simplified_Convert.cpp)
#target_link_libraries(Simplified_Convert PUBLIC root_event reconstructor)
#if(SKBUILD)
#    MESSAGE(STATUS "Installing Simplified_Convert")
#    install(TARGETS Simplified_Convert  DESTINATION pylast)
#endif()


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

