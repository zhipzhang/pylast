# Add additional source files for reconstructor extension
set(TMP_RECON_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/HillasSumWeightedReconstructor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DispWeightedReconstructor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EnergyRegressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ParticleClassifier.cpp
)

# Build the reconstructor_ext library
add_library(
    reconstructor_ext SHARED
    LGBMPredictor.cpp
    ParameterSchema.cpp
    LGBMModelLoader.cpp
    LGBMSigmaEstimator.cpp
)

# Find LightGBM library
find_library(
    LIGHTGBM_LIBRARY
    NAMES _lightgbm
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib
    NO_DEFAULT_PATH
)

if(LIGHTGBM_LIBRARY)
    message(STATUS "Found LightGBM library: ${LIGHTGBM_LIBRARY} in ${CMAKE_CURRENT_SOURCE_DIR}/lib")
else()
    if(DEFINED ENV{LIGHTGBM_LIBRARY})
        set(LIGHTGBM_LIBRARY $ENV{LIGHTGBM_LIBRARY})
        message(STATUS "Found LightGBM library from environment: ${LIGHTGBM_LIBRARY}")
    else()
        message(FATAL_ERROR "LightGBM library not found in ${CMAKE_CURRENT_SOURCE_DIR}/lib")
    endif()
endif()

# Find ONNX Runtime
find_package(onnxruntime CONFIG)

if(onnxruntime_FOUND)
    message(STATUS "Found onnxruntime: ${onnxruntime_DIR}")
else()
    message(STATUS "onnxruntime not found")
endif()

# Add test_onnx library if onnxruntime is found
if(onnxruntime_FOUND)
    add_library(test_onnx SHARED OnnxRunner.cpp)
    list(APPEND TMP_RECON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/TestReconstructor.cpp)
    target_link_libraries(test_onnx PUBLIC onnxruntime::onnxruntime)
    target_include_directories(test_onnx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
    if(SKBUILD)
        install(
            IMPORTED_RUNTIME_ARTIFACTS
                onnxruntime::onnxruntime
            LIBRARY DESTINATION pylast
        )
    endif()
endif()

# Pass TMP_RECON_FILES to parent scope
set(RECONSTURCTION_EXT_SOURCE_FILES ${TMP_RECON_FILES} PARENT_SCOPE)

# ROOT dependency
find_package(ROOT COMPONENTS Minuit2 REQUIRED)
target_include_directories(reconstructor_ext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link libraries to reconstructor_ext
if(onnxruntime_FOUND)
    target_link_libraries(
        reconstructor_ext
        PUBLIC reconstructor ${LIGHTGBM_LIBRARY} test_onnx ROOT::Minuit2
    )
else()
    target_link_libraries(
        reconstructor_ext
        PUBLIC reconstructor ${LIGHTGBM_LIBRARY} ROOT::Minuit2
    )
endif()

# Install targets if SKBUILD
if(SKBUILD)
    message(STATUS "Installing reconstructor_ext")
    install(TARGETS reconstructor_ext DESTINATION pylast)
    install(FILES ${LIGHTGBM_LIBRARY} DESTINATION pylast)
endif()