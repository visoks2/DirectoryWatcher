set(SQLite3_INSTALL_DIR ${CMAKE_THIRD_PARTY_DIR}/SQLite3)

FetchContent_Declare(
  sqlite3
    URL https://sqlite.org/2020/sqlite-autoconf-3340000.tar.gz
    URL_HASH SHA1=1544957cf4bcc9606aef541054b1cb59480a4b4e
    SOURCE_DIR ${SQLite3_INSTALL_DIR}
)
FetchContent_MakeAvailable(sqlite3)

include_directories(${SQLite3_INSTALL_DIR})
add_library(SQLite3 STATIC
    ${SQLite3_INSTALL_DIR}/sqlite3.c
)
# target_include_directories(SQLite3 PUBLIC 
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
#     ${CMAKE_THIRD_PARTY_DIR}/lib
# )
