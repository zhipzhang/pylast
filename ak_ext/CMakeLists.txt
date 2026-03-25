add_subdirectory(awkward-header)

# Interface library that bundles the awkward header-only targets and the
# pylast ak_ext include path.
add_library(ak_ext INTERFACE)
add_library(pylast::ak_ext ALIAS ak_ext)

target_link_libraries(ak_ext INTERFACE
    awkward::layout-builder
    awkward::growable-buffer
    awkward::builder-options
)

target_include_directories(ak_ext INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
