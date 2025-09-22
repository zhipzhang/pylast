
# Find DuckDB
include(FetchContent)
FetchContent_Declare(
    duckdb
    URL "https://gh.felicity.ac.cn/https://github.com/duckdb/duckdb/releases/download/v1.4.0/libduckdb-linux-amd64.zip"
    URL_HASH SHA256=3e7a902ee6cddf7d55035903cf26f14a9a45302bed6fbd379cf34e1c9cc9983f
)
FetchContent_MakeAvailable(duckdb)
FetchContent_GetProperties(duckdb)
if(duckdb_POPULATED)
file(COPY ${duckdb_SOURCE_DIR}/duckdb.hpp DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(COPY ${duckdb_SOURCE_DIR}/libduckdb.so DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib)
endif()
find_path(DUCKDB_INCLUDE_DIR duckdb.hpp PATHS include )
find_library(DUCKDB_LIBRARY duckdb PATHS lib)

add_library(database_supporter SHARED DatabaseWriter.cpp)
target_include_directories(database_supporter PUBLIC include)
target_link_libraries(database_supporter PUBLIC basic_event ${DUCKDB_LIBRARY})
