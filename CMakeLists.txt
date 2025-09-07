# project_root
# ├── CMakeLists.txt
# ├── src
# │   └── CMakeLists.txt
# ├── build
# │   └── CMakeCache.tx

# in case of missing target: 
# cmake -B ./build -S .
# cmake --build ./build --target targetchi


# source variables: CMAKE_CURRENT_SOURCE_DIR (current node) and CMAKE_SOURCE_DIR (tree root)


cmake_minimum_required(VERSION 3.10)
project(targetchi)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use vcpkg toolchain file if not already set
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/arthu/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()


# add subdirectories with their own Cmakelists.txt
add_subdirectory(raylib-gui/)
# add_subdirectory(bootloader/)


# Add source files
set(SOURCES
    src/main.cpp
    src/aimbuddy.cpp
    src/capture.cpp
    src/mouse.cpp
    src/show_gui.cpp
    src/init.cpp
    #src/config_gui.cpp
)

set(HEADERS
    src/aimbuddy.h
    src/capture.h
    src/mouse.h
    src/show_gui.h
    src/init.h
    #src/config_gui.h
)

# Add executable
add_executable(targetchi ${SOURCES} ${HEADERS})

set_target_properties(targetchi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Sets OpenCV_DIR to the path containing OpenCVConfig.cmake
set(OpenCV_DIR "C:/libs/opencv/build/x64/vc16/lib") 
# finds package in config mode- searches for <lowercasepackagename>-config.cmake or <package-name>Config.cmake
find_package(OpenCV REQUIRED)

target_include_directories(targetchi PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/raylib-gui/include
)

# target_link_directories(targetchi PRIVATE "C:\\libs\\raylib-5.5_win64_msvc16\\lib")

# PRIVATE-> if only used in .cpp, not .h files PUBLIC -> if used in any .h files, e.g for inheritance 
target_link_libraries(targetchi 
PRIVATE
    ${OpenCV_LIBS}
    raylib-gui
    hid setupapi
    #bootloader
)


# set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/bin")
install(TARGETS 
        targetchi
        RUNTIME 
            COMPONENT Runtime 
        )