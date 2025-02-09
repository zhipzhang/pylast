# Define a macro to conditionally add Python bindings
macro(add_python_binding MODULE_NAME SOURCES BINDING_FILES LIBRARIES)
    message(STATUS "Building Python binding for module '${MODULE_NAME}'")
    nanobind_add_module(${MODULE_NAME}
        STABLE_ABI
            ${SOURCES}
            ${BINDING_FILES}
        )
        target_link_libraries(${MODULE_NAME} PRIVATE ${LIBRARIES})
      # set_property(TARGET ${MODULE_NAME} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
        install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION pylast)
    get_target_property(lib_deps ${LIBRARIES} INTERFACE_LINK_LIBRARIES)
    if(lib_deps)
        install(TARGETS ${LIBRARIES} ${lib_deps} LIBRARY DESTINATION pylast)
    else()
        install(TARGETS ${LIBRARIES} LIBRARY DESTINATION pylast)
    endif()
endmacro()
set(BuildConfig "LACT_PROD0" CACHE STRING "Configuration to use for building hessio")
message(STATUS "hessioxxx BuildConfig='${BuildConfig}'")

add_subdirectory(bernlohr/LACT_hessioxxx)
add_definitions(-D${BuildConfig})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/)


set(LAST_ARRAY_EVENT_SOURCE_FILES
    R0Event.cpp
    R1Event.cpp
    EventMonitor.cpp
    SimulatedCamera.cpp
    TelMonitor.cpp
)
add_library(last_array_event SHARED ${LAST_ARRAY_EVENT_SOURCE_FILES})

set(BASIC_EVENT_SOURCE_FILES
    EventSource.cpp
    AtmosphereModel.cpp
    SimulationConfiguration.cpp
    Metaparam.cpp
    CameraGeometry.cpp
    CameraDescription.cpp
    CameraReadout.cpp
    OpticsDescription.cpp
    SubarrayDescription.cpp
    )
add_library(basic_event SHARED ${BASIC_EVENT_SOURCE_FILES})
target_link_libraries(basic_event PUBLIC last_array_event)

set(SIMTEL_SOURCE_FILES
    SimtelEventSource.cpp
    SimtelFileHandler.cpp)

set(CALIBRATION_SOURCE_FILES
    Calibration.cpp
    ImageExtractor.cpp)
add_library(calibrator  SHARED ${CALIBRATION_SOURCE_FILES})
add_library(simtel_event SHARED ${SIMTEL_SOURCE_FILES})
target_link_libraries(simtel_event PUBLIC basic_event hessio last_array_event calibrator)


if(BUILD_PYTHON_BINDINGS)
    set(BINDING_FILES
        pyarrayevent_bind.cpp
        pysubarray_bind.cpp
        pylast_bind.cpp
        pycalibrator_bind.cpp
    )
    set(MY_LIBS
        simtel_event 
        calibrator)
    add_python_binding(_pylast LoggerInitialize.cpp "${BINDING_FILES}" simtel_event)
endif()


add_executable(debug debug.cpp LoggerInitialize.cpp)
target_link_libraries(debug PRIVATE simtel_event)
