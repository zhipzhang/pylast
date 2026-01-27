set(RECONSTURCTION_EXT_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/HillasSumWeightedReconstructor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DispWeightedReconstructor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestReconstructor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EnergyRegressor.cpp
    PARENT_SCOPE)
add_library(reconstructor_ext SHARED 
            LGBMPredictor.cpp
            ParameterSchema.cpp)
find_library(LIGHTGBM_LIBRARY NAMES _lightgbm PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib NO_DEFAULT_PATH)

if(LIGHTGBM_LIBRARY)
    message(STATUS "Found LightGBM library: ${LIGHTGBM_LIBRARY}")
else()
     if(DEFINED ENV{LIGHTGBM_LIBRARY})
        set(LIGHTGBM_LIBRARY $ENV{LIGHTGBM_LIBRARY})
        message(STATUS "Found LightGBM library from environment: ${LIGHTGBM_LIBRARY}")
        else()
        message(FATAL_ERROR "LightGBM library not found in ${CMAKE_CURRENT_SOURCE_DIR}/lib")
     endif()
endif()


add_library(test_onnx SHARED OnnxRunner.cpp)
find_package(onnxruntime CONFIG REQUIRED)
target_link_libraries(test_onnx PUBLIC onnxruntime::onnxruntime)
target_include_directories(test_onnx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
if(SKBUILD)
install(IMPORTED_RUNTIME_ARTIFACTS
    onnxruntime::onnxruntime
    LIBRARY DESTINATION pylast
)
endif()

find_package(ROOT COMPONENTS Minuit2 REQUIRED)
target_include_directories(reconstructor_ext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(reconstructor_ext PUBLIC reconstructor ${LIGHTGBM_LIBRARY} test_onnx ROOT::Minuit2)
if(SKBUILD)
    MESSAGE(STATUS "Installing reconstructor_ext")
    install(TARGETS reconstructor_ext DESTINATION pylast)
    install(FILES ${LIGHTGBM_LIBRARY} DESTINATION pylast)
endif()