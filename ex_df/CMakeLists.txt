include(ExternalProject)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(ARROW_INSTALL_DIR ${CMAKE_BINARY_DIR}/arrow_install)

# 拉取并构建 Arrow
ExternalProject_Add(arrow_ep
    GIT_REPOSITORY https://github.com/apache/arrow.git
    GIT_TAG apache-arrow-21.0.0  
    PREFIX ${CMAKE_BINARY_DIR}/_deps/arrow
    SOURCE_SUBDIR cpp
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${ARROW_INSTALL_DIR}
        -DARROW_BUILD_SHARED=ON
        -DARROW_BUILD_STATIC=OFF
        -DARROW_BUILD_TESTS=OFF
        -DARROW_PARQUET=ON 
        -DARROW_COMPUTE=ON 
        -DARROW_DATASET=ON
        -DARROW_HDFS=ON
        -DARROW_FILESYSTEM=ON        
        -DARROW_CSV=ON
        -DARROW_JSON=ON
        -DARROW_WITH_BROTLI=ON
        -DARROW_WITH_BZ2=ON
        -DARROW_WITH_LZ4=ON 
        -DARROW_WITH_SNAPPY=ON 
        -DARROW_WITH_ZLIB=ON 
        -DARROW_WITH_ZSTD=ON
        -DPARQUET_REQUIRE_ENCRYPTION=ON
        -DCMAKE_INSTALL_LIBDIR=lib        
        -DPYTHON_EXECUTABLE=${Python_EXECUTABLE}
    BUILD_BYPRODUCTS ${ARROW_INSTALL_DIR}/lib/libarrow.so
)

set(ARROW_LIB_PATH ${ARROW_INSTALL_DIR}/lib)

ExternalProject_Get_Property(arrow_ep install_dir)
if(SKBUILD)
ExternalProject_Add_Step(arrow_ep install_lib_arrow
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${ARROW_LIB_PATH}/libarrow.so"
            "${CMAKE_INSTALL_PREFIX}/pylast/libarrow.so"
    DEPENDEES install
)
endif()

set(Arrow_DIR "${ARROW_INSTALL_DIR}/lib/cmake/arrow")

# 创建导入目标之前，先确保路径存在
if (NOT EXISTS "${ARROW_INSTALL_DIR}/lib/libarrow.so")
    message(WARNING "libarrow.so not found at expected path: ${ARROW_INSTALL_DIR}/lib/libarrow.so")
endif()

ExternalProject_Get_Property(arrow_ep source_dir)

message("Arrow source directory: ${source_dir}")
message("python: ${Python3_EXECUTABLE}")

add_library(Arrow::arrow SHARED IMPORTED GLOBAL)
set_target_properties(Arrow::arrow PROPERTIES
    IMPORTED_LOCATION ${ARROW_INSTALL_DIR}/lib/libarrow.so
)
add_dependencies(Arrow::arrow arrow_ep)

execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import pyarrow; import os; print(os.path.dirname(pyarrow.__file__))"
    OUTPUT_VARIABLE PYARROW_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Found pyarrow at: ${PYARROW_DIR}")

# pyarrow 的头文件
set(PYARROW_INCLUDE_DIR "${PYARROW_DIR}/include")
message(STATUS "Found pyarrow include at: ${PYARROW_INCLUDE_DIR}")

# pyarrow 的库（例如 libarrow_python.so）
set(PYARROW_LIB_DIR "${PYARROW_DIR}")
find_library(PYARROW_PYTHON_LIB NAMES arrow_python PATHS "${PYARROW_LIB_DIR}" NO_DEFAULT_PATH)

if (NOT PYARROW_PYTHON_LIB)
    message(FATAL_ERROR "libarrow_python.so not found in ${PYARROW_LIB_DIR}")
endif()

add_library(Arrow::arrow_python SHARED IMPORTED GLOBAL)
set_target_properties(Arrow::arrow_python PROPERTIES
    IMPORTED_LOCATION "${PYARROW_PYTHON_LIB}"
)

add_library(dataframe DataFrame.cpp)
target_include_directories(dataframe PUBLIC ${ARROW_INSTALL_DIR}/include ${PYARROW_INCLUDE_DIR} include)
target_link_libraries(dataframe PUBLIC Arrow::arrow  basic_event)

add_executable(test_data test_data.cpp)
target_link_libraries(test_data PUBLIC dataframe root_event)
