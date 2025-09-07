#pragma once

// Use Unicode throughout the application
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <Windows.h>
#include <string>
#include <functional>
#include <vector>
#include <map>

// Forward declarations
class ConfigGUI;

// mousetype enum for all possible mouse types
enum MouseType {
    BOOT_MOUSE = 0,        // [buttons][rel_x][rel_y]
    BWXY = 1,              // [buttons][wheel][rel_x][rel_y]
    BXYW = 2,              // [buttons][rel_x][rel_y][wheel]
    GLORIOUS = 3,          // [buttons][wheel][x_lo][ ][y_low][ ]

    // dont need
    LOGITECH_STANDARD = 4, // [buttons][rel_x][rel_y][wheel][pan]

    LOGITECH_HIRES = 5,    // [buttons][rel_x_lo][rel_x_hi][rel_y_lo][rel_y_hi][wheel_lo][wheel_hi]
    LOGITECH_GAMING = 6,   // [report_id][buttons_lo][buttons_hi][rel_x][rel_y][wheel][extra]
    LOGITECH_UNIFYING = 7, // [device_id][status][buttons][rel_x][rel_y][wheel][pan]
    GAMING_HIRES = 8,      // [buttons][rel_x_lo][rel_x_hi][rel_y_lo][rel_y_hi][wheel]
    MULTI_BUTTON = 9,      // [buttons_1][buttons_2][rel_x][rel_y][wheel]

    // dont need
    WIRELESS_BATT = 10,    // [buttons][rel_x][rel_y][wheel][battery]
    GESTURE = 11           // [buttons][rel_x][rel_y][wheel][gestures][gesture_data]
};

// Configuration struct to hold all settings
struct AimbuddyConfig {
    // General settings
    int QUIT;
    int TOGGLE_KEY;
    float INGAME_SENSITIVITY;

    // Aim settings
    int TOGGLE_MOVE;
    int ACTIVATE_MOVE;
    int XFOV;
    int YFOV;
    float NEW_FLICKSPEED;
    float MOVESPEED;

    // Trigger settings
    int TOGGLE_TRIGGER;
    int ACTIVATE_TRIGGER;
};

// Callback type for configuration completion
using ConfigCompletedCallback = std::function<void(const AimbuddyConfig&)>;

class ConfigGUI {
public:
    ConfigGUI(HINSTANCE hInstance, const AimbuddyConfig& initialConfig, ConfigCompletedCallback callback);
    ~ConfigGUI();

    // Show the configuration dialog
    bool Show();

private:
    // Window procedure for the main dialog
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Tab-specific drawing functions
    void DrawGeneralTab(HDC hdc);
    void DrawAimTab(HDC hdc);
    void DrawTriggerTab(HDC hdc);
    
    // Input handling
    void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Save configuration and close
    void SaveAndClose();
    
    // Convert key name to VK code and back
    int KeyNameToVK(const std::string& keyName);
    std::string VKToKeyName(int vk);
    
    // Helper methods to convert between string types
    std::wstring StringToWString(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }
    
    // Helper functions for drawing controls
    void DrawLabel(HDC hdc, int x, int y, const std::string& text);
    void DrawTextbox(HDC hdc, int id, int x, int y, int width, const std::string& text);
    void DrawButton(HDC hdc, int id, int x, int y, int width, int height, const std::string& text);
    void DrawTab(HDC hdc, int id, int x, int y, int width, int height, const std::string& text, bool selected);
    
    // Internal state
    HWND hwnd_;
    HINSTANCE hInstance_;
    AimbuddyConfig config_;
    ConfigCompletedCallback callback_;
    int currentTab_;
    int activeTextboxId_;

    // Change the state of the config_ 
    void AssignKey(int vk);
    void InitializeKeyMap();
    
    // Control IDs
    static const int ID_TAB_GENERAL = 100;
    static const int ID_TAB_AIM = 101;
    static const int ID_TAB_TRIGGER = 102;
    
    // Font and brush for MacOS '91 style
    HFONT hFont_;
    HBRUSH hBackgroundBrush_;
    
    // Key name to VK code mapping
    std::map<std::string, int> keyMap_;

    // function for mousetype packets
    void setMouseType(MouseType mousetype);
};

// Function to show config dialog and return settings
AimbuddyConfig ShowConfigDialog(HINSTANCE hInstance, const AimbuddyConfig& initialConfig);
