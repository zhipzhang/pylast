add_executable(Simplified_Convert Simplified_Convert.cpp)
target_link_libraries(Simplified_Convert PUBLIC root_event reconstructor)
if(SKBUILD)
    MESSAGE(STATUS "Installing Simplified_Convert")
    install(TARGETS Simplified_Convert  DESTINATION pylast)
endif()

