message(STATUS "Initialize root part of pylast")
add_definitions(
    -DMAXIMUM_TELESCOPES=32
    -DMAXIMUM_PIXELS=3000
    -DMAXIMUM_SECTORS=6000
    -DMAXIMUM_PIXELSECTORS=19
    -DMAXIMUM_DRAWERS=3000
    -DMAXIMUM_SLICES=128
)

find_package(ROOT REQUIRED COMPONENTS Core)
add_library(root_writer SHARED RootWriter.cpp RootDataLevels.cpp)
target_include_directories(root_writer PUBLIC ${ROOT_INCLUDE_DIRS} include)
target_link_libraries(root_writer PUBLIC data_writer ${ROOT_LIBRARIES})
add_library(root_event SHARED RootEventSource.cpp)
target_include_directories(root_event PUBLIC ${ROOT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(root_event PUBLIC ${ROOT_LIBRARIES} basic_event root_writer)

add_executable(test_root_write test_root.cpp RootWriter.cpp)
target_link_libraries(test_root_write PUBLIC root_writer simtel_event reconstructor image_processor calibrator)
add_executable(test_root_read test_root_read.cpp)
target_link_libraries(test_root_read PUBLIC root_event)

add_executable(test_root test_root.cpp RootWriter.cpp)
target_link_libraries(test_root PUBLIC root_event root_writer simtel_event reconstructor image_processor calibrator)

#add_subdirectory(ulities)