# Define a macro to conditionally add Python bindings
macro(add_python_binding MODULE_NAME  BINDING_FILES LIBRARIES DEPENDENCIES)
    message(STATUS "Building Python binding for module '${MODULE_NAME}'")
    nanobind_add_module(${MODULE_NAME}
        STABLE_ABI
            ${BINDING_FILES}
        )
    
        target_link_libraries(${MODULE_NAME} PRIVATE ${LIBRARIES})
      # set_property(TARGET ${MODULE_NAME} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
              # Generate stub file for each module
    nanobind_add_stub(
    ${MODULE_NAME}_stub
    MODULE ${MODULE_NAME}
    OUTPUT ${MODULE_NAME}.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:${MODULE_NAME}>
    DEPENDS ${MODULE_NAME} ${DEPENDENCIES}
    )
       install(TARGETS ${LIBRARIES} LIBRARY DESTINATION pylast)
       install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION pylast)
       install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.pyi DESTINATION pylast)
endmacro()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../root)
add_python_binding(_pylast_arrayevent  pyarrayevent_bind.cpp "" "")
add_python_binding(_pylast_subarray pysubarray_bind.cpp basic_event "")
add_python_binding(_pylast_imageprocessor pyimageprocessor_bind.cpp image_processor "")
add_python_binding(_pylast_calibrator pycalibrator_bind.cpp calibrator "")
set(PYEVENTSOURCE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LoggerInitialize.cpp
    pyeventsource_bind.cpp
)
set(DATAWRITER_LIBRARIES data_writer root_writer)
set(DATAWRITER_SOURCES
    pydatawriter_bind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../root/RootWriter.cpp
)

set(EVENTSOURCE_LIBRARIES basic_event simtel_event root_event)
add_python_binding(_pystatistic pystatistic_bind.cpp "" "")
add_python_binding(_pyeventsource "${PYEVENTSOURCE_SOURCES}" "${EVENTSOURCE_LIBRARIES}" _pylast_arrayevent)
add_python_binding(_pylast_showerprocessor pyshowerprocessor_bind.cpp reconstructor "")
add_python_binding(_pylast_datawriter "${DATAWRITER_SOURCES}" "${DATAWRITER_LIBRARIES}" "")


install(TARGETS query LIBRARY DESTINATION pylast)
install(TARGETS muparser LIBRARY DESTINATION pylast)
install(TARGETS hessio LIBRARY DESTINATION pylast)

if(HAVE_EVENTIO_EXTENSION)
    install(TARGETS eventio_extension LIBRARY DESTINATION pylast)
endif()