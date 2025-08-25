# Function to set appropriate definitions based on build configuration
function(set_hessio_definitions config)
  if(config STREQUAL "LACT_PROD0")
    add_definitions(
      -DMAXIMUM_TELESCOPES=32
      -DMAXIMUM_PIXELS=3000
      -DMAXIMUM_SECTORS=6000
      -DMAXIMUM_PIXELSECTORS=19
      -DMAXIMUM_DRAWERS=3000
      -DMAXIMUM_SLICES=128
    )
  endif()
endfunction()

# Apply the definitions based on the current BuildConfig

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bernlohr/LACT_hessioxxx/CMakeLists.txt")
  message("CMakeLists.txt not found in bernlohr/LACT_hessioxxx, Try to use git submodule update --init --recursive")
  find_package(Git QUIET)
  if(Git_FOUND)
    message(STATUS "Git found, trying to update submodules")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_RESULT
    )
    if(GIT_RESULT EQUAL 0)
      message(STATUS "Submodules updated successfully")
    else()
      message(FATAL_ERROR "Failed to update submodules, please check your git configuration")
    endif()
  else()
    message(FATAL_ERROR "Git not found, please install git to manage submodules")
  endif()
endif()
set_hessio_definitions("LACT_PROD0")
add_subdirectory(bernlohr/LACT_hessioxxx)
add_subdirectory(muparser)


find_package(CompressionHandler QUIET)
if(CompressionHandler_FOUND)
  message(STATUS "CompressionHandler found, Using eventio_extension")
  add_subdirectory(eventio_extension)
else()
  message(STATUS "CompressionHandler not found")
endif()


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

set(SIMTEL_SOURCE_FILES
    SimtelEventSource.cpp
    SimtelFileHandler.cpp
    LoggerInitialize.cpp)

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
if(CompressionHandler_FOUND)
  target_compile_definitions(simtel_event PRIVATE HAVE_EVENTIO_EXTENSION)
  target_link_libraries(simtel_event PUBLIC eventio_extension)
endif()

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
    MLReconstructor.cpp
    ShowerProcessor.cpp
    )
add_library(reconstructor SHARED ${RECONSTRUCTION_SOURCE_FILES} ${COORDINATES_SOURCE_FILES})
target_link_libraries(reconstructor PUBLIC basic_event query)



add_executable(debug debug.cpp LoggerInitialize.cpp)
target_link_libraries(debug PRIVATE simtel_event image_processor calibrator reconstructor)
