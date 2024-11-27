set(BuildConfig "LACT_PROD0" CACHE STRING "Configuration to use for building hessio")
message(STATUS "hessioxxx BuildConfig='${BuildConfig}'")


add_subdirectory(bernlohr/LACT_hessioxxx)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/)
nanobind_add_module(_pylast
    STABLE_ABI
    SimtelEventSource.cpp
)
target_link_libraries(_pylast PRIVATE hessio)
target_link_libraries(_pylast PUBLIC spdlog::spdlog)

# Ensure the RPATH is set correctly during installation
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
# Ensure the RPATH is preserved during installation
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


install(TARGETS _pylast LIBRARY DESTINATION pylast)
install(TARGETS hessio LIBRARY DESTINATION pylast)
