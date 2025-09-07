#pragma once

#include <string>
#include <functional>

// Forward declare the AimbuddyConfig struct to avoid including aimbuddy.h directly
struct AimbuddyConfig {
    // General tab
    int QUIT;                  // Quit key
    int TOGGLE_KEY;            // Toggle key
    float INGAME_SENSITIVITY;  // In-game sensitivity slider
    int MOUSETYPE;             // Mouse type (placeholder)
    
    // Aim tab
    int TOGGLE_MOVE;           // Toggle move key
    int ACTIVATE_MOVE;         // Activate move key
    int XFOV;                  // X field of view
    int YFOV;                  // Y field of view
    float NEW_FLICKSPEED;      // Flick speed
    float MOVESPEED;           // Move speed
    
    // Trigger tab
    int TOGGLE_TRIGGER;        // Toggle trigger key
    int ACTIVATE_TRIGGER;      // Activate trigger key
};

//typedef to prevent header conflicts with windows header
typedef struct OPAQUERECT {
    float x;                // Rectangle top-left corner position x
    float y;                // Rectangle top-left corner position y
    float width;            // Rectangle width
    float height;           // Rectangle height
} OPAQUERECT;

// Show the RayGUI config dialog with the given initial config
// Returns the updated config when the user clicks Save
AimbuddyConfig ShowRayGUIConfigDialog(const AimbuddyConfig& config);

// GUI implementation class (not exposed in public API)
class RayGUIConfig {
public:
    RayGUIConfig(const AimbuddyConfig& initialConfig, std::function<void(const AimbuddyConfig&)> onSaveCallback);
    void Show();
    
private:
    void InitWindow();
    void DrawGUI();
    void DrawGeneralTab();
    void DrawAimTab();
    void DrawTriggerTab();
    void CloseWindow();
    
    // Helper function to create a key binding button
    bool KeyBindButton(OPAQUERECT bounds, int* keyValue);
    // Convert virtual key to string representation
    const char* VirtualKeyToString(int vKey);
    
    AimbuddyConfig config;
    std::function<void(const AimbuddyConfig&)> saveCallback;
    
    bool windowShouldClose = false;
    int activeTab = 0;
    bool waitingForKeyInput = false;
    int* currentKeyBinding = nullptr;
    
    // Window dimensions
    const int windowWidth = 600;
    const int windowHeight = 300;
};
