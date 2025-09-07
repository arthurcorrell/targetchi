#include "../include/config_raygui.h"   // RayGUI implementation
#include "../include/init.h"

// Settings controlled by GUI

// Default configuration
AimbuddyConfig config = {
    'P',                  // QUIT
    0x12,             // TOGGLE_KEY (ALT)
    0.54f,               // INGAME_SENSITIVITY
    
    'Q',                  // TOGGLE_MOVE
    0x02,          // ACTIVATE_MOVE (Right mouse button)
    35,                  // XFOV
    31,                  // YFOV
    5.0f,                // NEW_FLICKSPEED
    2.0f / (5.0f * 0.54f), // MOVESPEED
    
    'W',                  // TOGGLE_TRIGGER
    0x01           // ACTIVATE_TRIGGER (Left mouse button)
};

// Constants derived from config
float FLICKSPEED;

int main(int argc, char* argv[]) {

    // Call blocking GUI to populate config
    config = ShowRayGUIConfigDialog(config);

}