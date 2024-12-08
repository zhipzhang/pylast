# Define a macro to conditionally add Python bindings
macro(add_python_binding MODULE_NAME SOURCES BINDING_FILES LIBRARIES)
    message(STATUS "Building Python binding for module '${MODULE_NAME}'")
    nanobind_add_module(${MODULE_NAME}
        STABLE_ABI
            ${SOURCES}
        )
        target_link_libraries(${MODULE_NAME} PRIVATE ${LIBRARIES})
        set_property(TARGET ${MODULE_NAME} APPEND PROPERTY INSTALL_RPATH "$ORIGIN")
        install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION pylast)
    install(TARGETS ${LIBRARIES} LIBRARY DESTINATION pylast)
endmacro()
set(BuildConfig "LACT_PROD0" CACHE STRING "Configuration to use for building hessio")
message(STATUS "hessioxxx BuildConfig='${BuildConfig}'")

add_subdirectory(bernlohr/LACT_hessioxxx)
add_definitions(-D${BuildConfig})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/)

set(SOURCE_FILES
    SimtelEventSource.cpp
    pylast_bind.cpp
    LoggerInitialize.cpp
    SimulationConfiguration.cpp
    EventSource.cpp
    AtmosphereModel.cpp
    SimtelFileHandler.cpp
    Metaparam.cpp
    SubarrayDescription.cpp
    CameraReadout.cpp
    CameraGeometry.cpp
    CameraDescription.cpp
    SimulatedCamera.cpp
    OpticsDescription.cpp
)

if(BUILD_PYTHON_BINDINGS)
    set(BINDING_FILES
        pylast_bind.cpp
        pyarrayevent_bind.cpp
        pysubarray_bind.cpp
    )
    add_python_binding(_pylast "${SOURCE_FILES}" "${BINDING_FILES}" hessio)
endif()

add_executable(debug debug.cpp SimtelEventSource.cpp LoggerInitialize.cpp SimulationConfiguration.cpp EventSource.cpp AtmosphereModel.cpp SimtelFileHandler.cpp Metaparam.cpp SubarrayDescription.cpp CameraReadout.cpp CameraGeometry.cpp CameraDescription.cpp OpticsDescription.cpp   SimulatedCamera.cpp)
target_link_libraries(debug PRIVATE hessio)
