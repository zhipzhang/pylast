find_package(ROOT REQUIRED COMPONENTS Core)
add_library(prototype_event SHARED PrototypeEventSource.cpp PrototypeCalibrator.cpp)
target_include_directories(prototype_event PUBLIC ${ROOT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(prototype_event PUBLIC ${ROOT_LIBRARIES} basic_event)