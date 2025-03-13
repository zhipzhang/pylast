set(BuildConfig "LACT_PROD0" CACHE STRING "Configuration to use for building hessio")
message(STATUS "hessioxxx BuildConfig='${BuildConfig}'")

add_subdirectory(bernlohr/LACT_hessioxxx)
add_subdirectory(muparser)
add_definitions(-D${BuildConfig})


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
set(IMAGE_PROCESSOR_SOURCE_FILES
    ImageProcessor.cpp
    ImageCleaner.cpp)
add_library(image_processor SHARED ${IMAGE_PROCESSOR_SOURCE_FILES})
target_link_libraries(image_processor PUBLIC basic_event)

add_library(simtel_event SHARED ${SIMTEL_SOURCE_FILES})
target_link_libraries(simtel_event PUBLIC basic_event hessio)
target_include_directories(simtel_event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/)

set(DATA_WRITER_SOURCE_FILES
    DataWriter.cpp
    )
add_library(data_writer SHARED ${DATA_WRITER_SOURCE_FILES})
target_link_libraries(data_writer PUBLIC basic_event)
set(QUERY_SOURCE_FILES
    ImageQuery.cpp
    )
add_library(query SHARED ${QUERY_SOURCE_FILES})
target_link_libraries(query PUBLIC muparser::muparser)

set (COORDINATES_SOURCE_FILES
    Coordinates.cpp
    CoordFrames.cpp
    )
set (RECONSTRUCTION_SOURCE_FILES
    GeometryReconstructor.cpp
    HillasReconstructor.cpp
    ShowerProcessor.cpp
    )
add_library(reconstructor SHARED ${RECONSTRUCTION_SOURCE_FILES} ${COORDINATES_SOURCE_FILES})
target_link_libraries(reconstructor PUBLIC basic_event query)



add_executable(debug debug.cpp LoggerInitialize.cpp)
target_link_libraries(debug PRIVATE simtel_event image_processor calibrator reconstructor)
