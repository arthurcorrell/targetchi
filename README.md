# AimBuddy C++ Version

This is a C++ translation of the original Python AimBuddy project.

## Requirements

- Windows OS
- OpenCV 4.x
- CMake 3.10 or newer
- Visual Studio with C++17 support (or any C++17 compatible compiler)

## Dependencies

- OpenCV for image processing
- Windows API for screen capture
- HID API for Arduino mouse interface

## Build Instructions

### Using CMake:

1. Make sure you have OpenCV installed and properly configured
2. Create a build directory:
```
mkdir build
cd build
```

3. Configure with CMake:
```
cmake ..
```

4. Build the project:
```
cmake --build . --config Release
```

5. The executable will be in the `bin/Release` directory

### Using Visual Studio:

1. Open the project folder in Visual Studio with CMake support
2. Configure CMake settings if needed
3. Build the solution

## Usage

1. Run the application
2. Press F1 to toggle AimBuddy on/off
3. Use right mouse button for Aimbot, left Alt for Triggerbot, and left Ctrl for Silentaim

## Controls

- F1: Toggle AimBuddy on/off
- F2: Toggle detection window
- Right Mouse Button: Activate Aimbot
- Left Alt: Activate Triggerbot
- Left Ctrl: Activate Silentaim
- Q: Quit application

## Note

This application requires an Arduino Leonardo (or compatible board) configured as a HID mouse device.
