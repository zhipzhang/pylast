set(BuildConfig "LACT_PROD0" CACHE STRING "Configuration to use for building hessio")
message(STATUS "hessioxxx BuildConfig='${BuildConfig}'")


add_subdirectory(bernlohr/LACT_hessioxxx)
add_definitions(-D${BuildConfig})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/)
nanobind_add_module(_pylast
    STABLE_ABI
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
    OpticsDescription.cpp
    pysubarray_bind.cpp
)
target_link_libraries(_pylast PRIVATE hessio)
set_property(TARGET _pylast APPEND PROPERTY INSTALL_RPATH "$ORIGIN")

install(TARGETS _pylast LIBRARY DESTINATION pylast)
install(TARGETS hessio LIBRARY DESTINATION pylast)
