add_library(eventio_extension SHARED EventIOWrapper.cpp EventIOHandler.cpp)
target_link_libraries(eventio_extension PUBLIC CompressionHandler::CompressionHandler)
target_include_directories(eventio_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# Set RPATH settings for the library
#set_target_properties(eventio_extension PROPERTIES
#    INSTALL_RPATH "$ORIGIN"
#    BUILD_WITH_INSTALL_RPATH TRUE
#    SKIP_BUILD_RPATH FALSE
#    INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set a global variable to indicate that we have the eventio_extension
set(HAVE_EVENTIO_EXTENSION TRUE CACHE BOOL "EventIO extension is available" FORCE)