cmake_minimum_required(VERSION 3.15)

project(LAST)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (NOT SKBUILD)
  message(WARNING "\
  This CMake file is meant to be executed using 'scikit-build-core'.
  Running it directly will almost certainly not produce the desired
  result. If you are a user trying to install this package, use the
  command below, which will install all necessary build dependencies,
  compile the package in an isolated environment, and then install it.
  =====================================================================
   $ pip install .
  =====================================================================
  If you are a software developer, and this is your own package, then
  it is usually much more efficient to install the build dependencies
  in your environment once and use the following command that avoids
  a costly creation of a new virtual environment at every compilation:
  =====================================================================
   $ pip install nanobind scikit-build-core[pyproject]
   $ pip install --no-build-isolation -ve .
  =====================================================================
  You may optionally add -Ceditable.rebuild=true to auto-rebuild when
  the package is imported. Otherwise, you need to rerun the above
  after editing C++ files.")
endif()
if(SKBUILD)
    message(STATUS "Building Python bindings with scikit-build-core")
    set(BUILD_PYTHON_BINDINGS ON)
    set(WITH_ROOT ON)
endif()
# Try to import all Python components potentially needed by nanobind
if(BUILD_PYTHON_BINDINGS)
find_package(Python 3.8
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule)
find_package(nanobind CONFIG REQUIRED)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/external)
# Add warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall              # Enable all warnings
        -Wextra            # Enable extra warnings
        -Wpedantic         # Strict ISO C/C++
        -Wuninitialized    # Warn about uninitialized variables
        -Wmaybe-uninitialized # Warn about potentially uninitialized variables
    )
endif()
if(APPLE)
    set(BASEPOINT @loader_path)
else()
    set(BASEPOINT $ORIGIN)
endif()
set(CMAKE_INSTALL_RPATH ${BASEPOINT} ${BASEPOINT}/${CMAKE_INSTALL_LIBDIR})
# Option for Intel MKL support
option(FAST "Enable Intel MKL/OpenMp support"  OFF)
if(FAST)
    message(STATUS "Building with Intel MKL and OpenMP support for performance optimization")
    # Find required packages
    find_package(OpenMP)
    # Configure OpenMP if found
    if(OpenMP_CXX_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    else()
        message(WARNING "OpenMP requested but not found. Building without OpenMP parallelization.")
    endif()
endif()

add_subdirectory(src)

if(WITH_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(root)
    add_subdirectory(test)
endif()
if(WITH_ROOT)
    add_subdirectory(root)
endif()

if(SKBUILD)
    add_subdirectory(binding)
endif()
