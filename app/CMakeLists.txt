cmake_minimum_required (VERSION 3.12)

# add_subdirectory(src)

add_executable(MyApp 
    main.cpp  
    fs/watcher.cpp
)

# target_link_libraries (
#     Server 
#     ${CMAKE_THREAD_LIBS_INIT} 
#     TCPServerLib
#     CommonLib
# )

 