#pragma once

#ifdef RAYLIB_GUI_EXPORTS
    #define RAYLIB_GUI_API __declspec(dllexport)
#else
    #define RAYLIB_GUI_API __declspec(dllimport)
#endif

#include <string>
#include <functional>

// Forward declare the AimbuddyConfig struct to avoid including aimbuddy.h directly
struct RAYLIB_GUI_API AimbuddyConfig {
    // General tab
    int QUIT;                  // Quit key
    int TOGGLE_KEY;            // Toggle key
    
    // Aim tab
    int TOGGLE_MOVE;           // Toggle move key
    int ACTIVATE_MOVE;         // Activate move key
    int XFOV;                  // X field of view
    int YFOV;                  // Y field of view
    float MOVESPEED;           // Move speed
    
    // Trigger tab
    int TOGGLE_TRIGGER;        // Toggle trigger key
    int ACTIVATE_TRIGGER;      // Activate trigger key
    bool WASD_SAFETY; // checkbox to ensure cancelled out movement keys
    int ACTIVE_MOUSE; // 0-indexed MouseType
    int COLOR_MASK; // 0=P, 1=R, 2=Y for color masking
};

// Show the RayGUI config dialog with the given initial config
// Returns the updated config when the user clicks Save
RAYLIB_GUI_API AimbuddyConfig ShowRayGUIConfigDialog(const AimbuddyConfig& config);


//typedef to prevent header conflicts with windows header
typedef struct OPAQUERECT {
    float x;                // Rectangle top-left corner position x
    float y;                // Rectangle top-left corner position y
    float width;            // Rectangle width
    float height;           // Rectangle height
} OPAQUERECT;


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
    void DrawMouseSelectorDialog();
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
    bool mouseSelector = false;
    bool errorDialog = false;
    bool quitTargetchi = false;
    int spinnerValue1 = 0;
    int spinnerValue2 = 0;
    bool showPollingInfoBox = false;
    
    // Window dimensions
    const int windowWidth = 600;
    const int windowHeight = 300;
};
