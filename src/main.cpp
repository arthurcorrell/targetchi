// Choose which GUI implementation to use
// #include "config_gui.h"  // Windows native GUI
#include "config_raygui.h"   // RayGUI implementation
#include "aimbuddy.h"


#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <Windows.h>


// Settings controlled by GUI

// Default configuration
AimbuddyConfig config = {
    'P',                  // QUIT
    'Q',             // TOGGLE_KEY (ALT)
    0.54f,               // INGAME_SENSITIVITY
    
    'W',                  // TOGGLE_MOVE
    'E',          // ACTIVATE_MOVE (Right mouse button)
    99,                  // XFOV
    99,                  // YFOV
    5.0f,                // NEW_FLICKSPEED
    2.0f / (5.0f * 0.54f), // MOVESPEED
    
    'W',                  // TOGGLE_TRIGGER
    VK_LBUTTON,           // ACTIVATE_TRIGGER (Left mouse button)
    true,                  // WASD_SAFETY
    3,                    // 0-indexed MouseType
};

// Constants derived from config
float FLICKSPEED;

int main(int argc, char* argv[]) {

    // Call blocking GUI to populate config
    config = ShowRayGUIConfigDialog(config);
    
    // Update derived values
    FLICKSPEED = config.NEW_FLICKSPEED;
    
    // Get screen dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    int CENTER_X = screen_width / 2;
    int CENTER_Y = screen_height / 2;
    
    AimBuddy aimbuddy(CENTER_X - config.XFOV / 2, CENTER_Y - config.YFOV / 2, 
                     config.XFOV, config.YFOV, FLICKSPEED, config.MOVESPEED,
                     config.ACTIVATE_MOVE, config.ACTIVATE_TRIGGER);
    
    std::cout << "Targetchi started with configured settings" << std::endl;
    std::cout << "Press " << (char)config.QUIT << " to quit" << std::endl;
    
    std::string status_move = "Disabled";
    std::string status_trigger = "Disabled";
    std::atomic<bool> running(true);
    
    // Capture a local copy of the quit key to avoid capturing global config
    int quitKey = config.QUIT;
    
    // Create a separate thread to check for keyboard interrupts
    std::thread interrupt_thread([&running, quitKey]() {
        while (running) {
            if (GetAsyncKeyState(quitKey) & 0x8000) {
                running = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    try {
        while (running) {
            if (GetAsyncKeyState(config.TOGGLE_KEY)) {
                if (GetAsyncKeyState(config.TOGGLE_MOVE) & 0x8000) {
                    aimbuddy.toggle_move();
                    status_move = aimbuddy.toggled_move ? "Enabled " : "Disabled";
                }
                if (GetAsyncKeyState(config.TOGGLE_TRIGGER) & 0x8000) {
                    aimbuddy.toggle_trigger();
                    status_trigger = aimbuddy.toggled_trigger ? "Enabled " : "Disabled";
                }
            }
            std::cout << "Status move: " << status_move << std::endl;
            std::cout << "Status trigger " << status_trigger << std::endl;
            std::cout << std::flush;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error:";
        std::cout << std::endl;
    }
    
    std::cout << "Exiting" << std::endl;
    std::cout << std::endl;
    
    running = false;
    if (interrupt_thread.joinable()) {
        interrupt_thread.join();
    }
    
    return 0;
}
