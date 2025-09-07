cmake_minimum_required(VERSION 3.10)
project(raylib-gui)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/main.cpp
    src/config_raygui.cpp
    src/init.cpp
)

set(HEADERS
    src/config_raygui.h
    src/init.h
    src/raygui.h
)

# SHARED library (linked obj files) 
# MODULE library (SHARED, but may not be linked by other binaries as deps)
# OBJECT library (unlinked group of obj files)
# STATIC library (OBJECT, to be linked into the executable) 
add_executable(raylib-gui ${SOURCES} ${HEADERS})

set_target_properties(raylib-gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")


# Set raylib directory
set(RAYLIB_DIR "C:\\libs\\raylib-5.5_win64_msvc16")

# Include raylib directories
include_directories(${RAYLIB_DIR}/include/)

# Define public include directories
# target_include_directories(raylib-gui 
#     PUBLIC
#         ${CMAKE_CURRENT_SOURCE_DIR}/src
# )
target_link_directories(raylib-gui PRIVATE "C:\\libs\\raylib-5.5_win64_msvc16\\lib")

# Link with raylib 
target_link_libraries(raylib-gui PRIVATE raylib winmm hid setupapi)

# Define RAYGUI_IMPLEMENTATION in exactly ONE source file
# target_compile_definitions(raylib-gui PRIVATE RAYGUI_IMPLEMENTATION)

# link libraries PRIVATELY to prevent conflict with WinUser API, PUBLICLY to allow transitive linking
# target_link_libraries(raylib-gui 
#     PRIVATE  
#         winmm gdi32 user32 shell32
#         setupapi hid
#         )



# Copy DLLs to build directory if needed
# add_custom_command(TARGET raygui_demo POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "${RAYLIB_DIR}/bin/raylib.dll"
#    $<TARGET_FILE_DIR:raygui_demo>)
