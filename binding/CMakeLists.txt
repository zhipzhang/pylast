# Define a macro to conditionally add Python bindings
macro(add_python_binding MODULE_NAME  BINDING_FILES LIBRARIES)
    message(STATUS "Building Python binding for module '${MODULE_NAME}'")
    nanobind_add_module(${MODULE_NAME}
        STABLE_ABI
            ${BINDING_FILES}
        )
        target_link_libraries(${MODULE_NAME} PRIVATE ${LIBRARIES})
      # set_property(TARGET ${MODULE_NAME} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
        install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION pylast)
        install(TARGETS ${LIBRARIES} LIBRARY DESTINATION pylast)
endmacro()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../root)
add_python_binding(_pylast_arrayevent  pyarrayevent_bind.cpp last_array_event)
add_python_binding(_pylast_subarray pysubarray_bind.cpp basic_event)
add_python_binding(_pylast_imageprocessor pyimageprocessor_bind.cpp image_processor)
add_python_binding(_pylast_calibrator pycalibrator_bind.cpp calibrator)
set(PYEVENTSOURCE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/LoggerInitialize.cpp
    pyeventsource_bind.cpp
)
set(DATAWRITER_LIBRARIES data_writer root_writer)
set(DATAWRITER_SOURCES
    pydatawriter_bind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../root/RootWriter.cpp
)

add_python_binding(_pyeventsource "${PYEVENTSOURCE_SOURCES}" basic_event)
add_python_binding(_pysimtelsource pysimtelsource_bind.cpp simtel_event)
add_python_binding(_pylast_showerprocessor pyshowerprocessor_bind.cpp reconstructor)
add_python_binding(_pylast_datawriter "${DATAWRITER_SOURCES}" "${DATAWRITER_LIBRARIES}")
add_python_binding(_pystatistic pystatistic_bind.cpp "")

add_python_binding(_pyrooteventsource  pyrooteventsource_bind.cpp root_event)
install(TARGETS query LIBRARY DESTINATION pylast)
install(TARGETS muparser LIBRARY DESTINATION pylast)
install(TARGETS hessio LIBRARY DESTINATION pylast)
