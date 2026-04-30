# Define a macro to conditionally add Python bindings, with an option to generate stubs (default: ON)
macro(add_python_binding MODULE_NAME BINDING_FILES LIBRARIES DEPENDENCIES)
    # Support legacy 4-argument call
    set(_ADD_STUB TRUE)
    if(${ARGC} GREATER 4)
        set(_ADD_STUB ${ARGV4})
    endif()
    message(STATUS "Building Python binding for module '${MODULE_NAME}' (stub: ${_ADD_STUB})")
    nanobind_add_module(${MODULE_NAME}
        STABLE_ABI
        ${BINDING_FILES}
    )

    target_link_libraries(${MODULE_NAME} PRIVATE ${LIBRARIES})
    # set_property(TARGET ${MODULE_NAME} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")

    if(_ADD_STUB)
        # Generate stub file for each module
        nanobind_add_stub(
            ${MODULE_NAME}_stub
            MODULE ${MODULE_NAME}
            OUTPUT ${MODULE_NAME}.pyi
            PYTHON_PATH $<TARGET_FILE_DIR:${MODULE_NAME}>
            DEPENDS ${MODULE_NAME} ${DEPENDENCIES}
        )
    endif()
    
    install(TARGETS ${LIBRARIES} LIBRARY DESTINATION pylast)
    install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION pylast)

    if(_ADD_STUB)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.pyi DESTINATION pylast)
    endif()
endmacro()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../root)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../calibration)
add_python_binding(_pylast_arrayevent  pyarrayevent_bind.cpp "" "")
add_python_binding(_pylast_subarray pysubarray_bind.cpp basic_event "")
add_python_binding(_pylast_imageprocessor pyimageprocessor_bind.cpp image_processor "")

set(CALIBRATOR_LIBRARIES calibrator prototype_calibrator)
set(CALIBRATOR_SOURCES
    pycalibrator_bind.cpp
)
add_python_binding(_pylast_calibrator "${CALIBRATOR_SOURCES}" "${CALIBRATOR_LIBRARIES}" "")
set(PYEVENTSOURCE_SOURCES
    pyeventsource_bind.cpp
)
set(DATAWRITER_LIBRARIES data_writer root_writer)
set(DATAWRITER_SOURCES
    pydatawriter_bind.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../root/RootWriter.cpp
)

set(EVENTSOURCE_LIBRARIES basic_event simtel_event root_event prototype_event)
add_python_binding(_pystatistic pystatistic_bind.cpp "" "")
add_python_binding(_pyeventsource "${PYEVENTSOURCE_SOURCES}" "${EVENTSOURCE_LIBRARIES}" _pylast_arrayevent)

# ak_ext is header-only (INTERFACE); link it after the macro to avoid
# passing it to install(TARGETS … LIBRARY …) which only applies to real libs.
set(AKARRAY_LIBRARIES basic_event simtel_event root_event)
add_python_binding(_pylast_akarray pyakarray_bind.cpp "${AKARRAY_LIBRARIES}" _pyeventsource)
target_link_libraries(_pylast_akarray PRIVATE ak_ext)
if(WITH_EXT_REC)
    set(RECONSTRUCTOR_LIBRARIES reconstructor reconstructor_ext)
else()
    set(RECONSTRUCTOR_LIBRARIES reconstructor)
endif()
set(RECONSTRUCTOR_SOURCES pyshowerprocessor_bind.cpp ${RECONSTURCTION_EXT_SOURCE_FILES})
add_python_binding(_pylast_showerprocessor "${RECONSTRUCTOR_SOURCES}" "${RECONSTRUCTOR_LIBRARIES}" "")
add_python_binding(_pylast_datawriter "${DATAWRITER_SOURCES}" "${DATAWRITER_LIBRARIES}" "")


install(TARGETS query LIBRARY DESTINATION pylast)
install(TARGETS muparser LIBRARY DESTINATION pylast)
install(TARGETS hessio LIBRARY DESTINATION pylast)
if(HAVE_EVENTIO_EXTENSION)
    install(TARGETS eventio_extension LIBRARY DESTINATION pylast)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config DESTINATION pylast)