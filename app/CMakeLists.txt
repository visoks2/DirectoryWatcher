cmake_minimum_required (VERSION 3.12)

message(STATUS "${SQLite3_INCLUDE_DIRS}")
include_directories(MyApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${SQLite3_INCLUDE_DIRS})

add_executable(MyApp 
    main.cpp  
    App.cpp  
    fs/Watcher.cpp
    fs/Database.cpp
)

target_link_libraries (MyApp SQLite3)
