#include "../include/config_raygui.h"
#include "raygui.h"
#include <string>
#include <map>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// include globally instantiated DeviceObject
#include "../include/init.h"

// Map of virtual key codes to string representations
static std::map<int, const char*> keyNames = {
    {0x01, "LMB"},
    {0x02, "RMB"},
    {0x04, "MMB"},
    {0x08, "BACK"},
    {0x09, "TAB"},
    {0x0D, "ENTER"},
    {0x10, "SHIFT"},
    {0x11, "CTRL"},
    {0x12, "ALT"},
    {0x13, "PAUSE"},
    {0x14, "CAPS"},
    {0x1B, "ESC"},
    {0x20, "SPACE"},
    {0x25, "LEFT"},
    {0x26, "UP"},
    {0x27, "RIGHT"},
    {0x28, "DOWN"},
    {0x30, "0"},
    {0x31, "1"},
    {0x32, "2"},
    {0x33, "3"},
    {0x34, "4"},
    {0x35, "5"},
    {0x36, "6"},
    {0x37, "7"},
    {0x38, "8"},
    {0x39, "9"},
    {0x41, "A"},
    {0x42, "B"},
    {0x43, "C"},
    {0x44, "D"},
    {0x45, "E"},
    {0x46, "F"},
    {0x47, "G"},
    {0x48, "H"},
    {0x49, "I"},
    {0x4A, "J"},
    {0x4B, "K"},
    {0x4C, "L"},
    {0x4D, "M"},
    {0x4E, "N"},
    {0x4F, "O"},
    {0x50, "P"},
    {0x51, "Q"},
    {0x52, "R"},
    {0x53, "S"},
    {0x54, "T"},
    {0x55, "U"},
    {0x56, "V"},
    {0x57, "W"},
    {0x58, "X"},
    {0x59, "Y"},
    {0x5A, "Z"},
    // Add more key mappings as needed
};

// Wrapper function that creates and shows the GUI
AimbuddyConfig ShowRayGUIConfigDialog(const AimbuddyConfig& initialConfig) {
    // Create a copy of the config that will be returned
    AimbuddyConfig resultConfig = initialConfig;

    // callback function ensures saving config only when save button is used
    auto saveCallback = [&resultConfig](const AimbuddyConfig& updatedConfig) {
        resultConfig = updatedConfig;
    };
    
    RayGUIConfig gui(initialConfig, saveCallback);
    gui.Show();
    
    // Return the updated config
    return resultConfig;
}

// RayGUIConfig implementation
RayGUIConfig::RayGUIConfig(const AimbuddyConfig& initialConfig, 
                           std::function<void(const AimbuddyConfig&)> onSaveCallback)
    : config(initialConfig), saveCallback(onSaveCallback), activeTab(0), mouseSelector(false), errorDialog(false), quitTargetchi(false) {

    }

void RayGUIConfig::Show() {
    InitWindow();
    
    while (!windowShouldClose) {
        // Process input
        if (IsKeyPressed(KEY_ESCAPE) && !waitingForKeyInput) {
            windowShouldClose = true;
        }
        
        // Handle key binding input if waiting
        if (waitingForKeyInput && currentKeyBinding != nullptr) {
            for (int i = 0; i < 256; i++) {
                if (IsKeyPressed(i)) {
                    *currentKeyBinding = i;
                    waitingForKeyInput = false;
                    currentKeyBinding = nullptr;
                    break;
                }
            }
            
            // Also check mouse buttons
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                *currentKeyBinding = 0x01;  // VK_LBUTTON
                waitingForKeyInput = false;
                currentKeyBinding = nullptr;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                *currentKeyBinding = 0x02;  // VK_RBUTTON
                waitingForKeyInput = false;
                currentKeyBinding = nullptr;
            }
            else if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                *currentKeyBinding = 0x04;  // VK_MBUTTON
                waitingForKeyInput = false;
                currentKeyBinding = nullptr;
            }
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawGUI();
        EndDrawing();
    }
    
    CloseWindow();
}

void RayGUIConfig::InitWindow() {
    ::InitWindow(windowWidth, windowHeight, "Targetchi Configuration");
    SetTargetFPS(60);
    GuiLoadStyleDefault();
}

void RayGUIConfig::DrawGUI() {
    // Main container
    Rectangle mainPanelRec = {0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight)};
    GuiPanel(mainPanelRec, "Targetchi Configuration");

    const char* tabs[] {"General", "Aim", "Trigger"};
    Rectangle tabRect = { 10.0f, 30.0f, 186.7f, 25.0f };
    
    for (int i=0; i <3; i++) {
        Rectangle activeRect = { (tabRect.x * (i+1))+ (tabRect.width * i), tabRect.y, tabRect.width, tabRect.height};
        
        
        if (GuiButton(activeRect, tabs[i])) {
            activeTab = i;
        }
    } 

    
    // Content area
    Rectangle contentRect = {10.0f, 65.0f, static_cast<float>(windowWidth - 20), static_cast<float>(windowHeight - 105)};
    GuiPanel(contentRect, NULL);

    // mouseSelector window
    if (mouseSelector) {
        activeTab = 3;
    }
    
    // Draw the active tab content
    switch (activeTab) {
        case 0: DrawGeneralTab(); break;
        case 1: DrawAimTab(); break;
        case 2: DrawTriggerTab(); break;
        case 3: DrawMouseSelectorDialog(); break;
    }
    
    // Save button at the bottom right
    Rectangle saveButtonRect = {static_cast<float>(windowWidth - 110), static_cast<float>(windowHeight - 30), 100.0f, 20.0f};
    if (GuiButton(saveButtonRect, "START")) {
        // Call the save callback with the updated config
        if (saveCallback) {
            saveCallback(config);
        }
        if (!(DeviceConfig.activeHandle)) {
            DeviceConfig.startHandle();
        }
        if (!(DeviceConfig.deviceError)) {
            DeviceConfig.setSaveByte(1);
            windowShouldClose = true;
        } else {
            errorDialog = true;
        }
    }
    // Quit button at the bottom left
    Rectangle quitButtonRect = {static_cast<float>(10), static_cast<float>(windowHeight - 30), 100.0f, 20.0f};
    if (GuiButton(quitButtonRect, "QUIT")) {
        // set windowShouldClose, which handles any still active handles
        windowShouldClose = true;
        quitTargetchi = true;
    }

    // draw error dialog if error has been set
    if (errorDialog) {
        Rectangle errorRect = {contentRect.x + (contentRect.width / 3), contentRect.y + (contentRect.height / 2), (contentRect.width / 3), (contentRect.height / 3)};
        int result = GuiMessageBox(contentRect, "#191#DRIVER ERROR", "Could not find connected device. Make sure device is connected and flashed", "OK");
        if (result >= 0) {
            errorDialog = false;
        }
    }
}

void RayGUIConfig::DrawMouseSelectorDialog() {
    Rectangle contentRect = {10.0f, 65.0f, static_cast<float>(windowWidth - 20), static_cast<float>(windowHeight - 105)};
    int result = GuiMessageBox(contentRect,
        GuiIconText(ICON_CURSOR_POINTER, "Mouse Config"), "Select a mouse config", "BOOT;1;2;3;4;5;6;7;8");
    if (result == 0) {
        mouseSelector = false;
        activeTab = 0;
    }
    if (result > 0) {
        // which button to display as pressed [0, 6]
        config.ACTIVE_MOUSE = result-1;

        // send 0-indexed mouse config, -1 for no change
        DeviceConfig.setMouseByte(config.ACTIVE_MOUSE);
    }
}
    
void RayGUIConfig::DrawGeneralTab() {
    int startY = 85;
    int spacing = 30;
    int labelWidth = 150;
    int controlWidth = 120;
    int leftMargin = 20;
    
    // QUIT key binding
    Rectangle quitLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(quitLabelRec, "Quit Key:");
    OPAQUERECT quitButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(quitButtonRec, &config.QUIT);
    
    // TOGGLE_KEY binding
    Rectangle toggleKeyLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(toggleKeyLabelRec, "Toggle Key:");
    OPAQUERECT toggleKeyButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(toggleKeyButtonRec, &config.TOGGLE_KEY);
    
    // INGAME_SENSITIVITY slider
    Rectangle sensitivityLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*2), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(sensitivityLabelRec, "In-game Sensitivity:");
    float sensitivity = config.INGAME_SENSITIVITY;
    Rectangle sensitivitySliderRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*2), static_cast<float>(controlWidth), 20.0f};
    GuiSlider(sensitivitySliderRec, NULL, TextFormat("%.2f", sensitivity), &sensitivity, 0.1f, 2.0f);
    config.INGAME_SENSITIVITY = sensitivity;
    
    // MOUSETYPE placeholder
    Rectangle mouseTypeLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*3), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(mouseTypeLabelRec, "Mouse Type:");
    Rectangle mouseTypeButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*3), static_cast<float>(controlWidth), 20.0f};
    if (GuiButton(mouseTypeButtonRec, GuiIconText(ICON_CURSOR_POINTER, "Configure Mouse"))) {
        // flow one: start device handle upon opening mouse configuration
        if (!(DeviceConfig.activeHandle)) {
            DeviceConfig.startHandle();
        }
        if (!(DeviceConfig.deviceError)) {
            mouseSelector = true;
        } else {
            errorDialog = true;
        }
    }
}

void RayGUIConfig::DrawAimTab() {
    int startY = 85;
    int spacing = 30;
    int labelWidth = 150;
    int controlWidth = 120;
    int leftMargin = 20;
    
    // TOGGLE_MOVE key binding
    Rectangle toggleMoveLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(toggleMoveLabelRec, "Toggle Move Key:");
    OPAQUERECT toggleMoveButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(toggleMoveButtonRec, &config.TOGGLE_MOVE);
    
    // ACTIVATE_MOVE key binding
    Rectangle activateMoveLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(activateMoveLabelRec, "Activate Move Key:");
    OPAQUERECT activateMoveButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(activateMoveButtonRec, &config.ACTIVATE_MOVE);
    
    // XFOV slider
    Rectangle xfovLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*2), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(xfovLabelRec, "X Field of View:");
    float xfov = static_cast<float>(config.XFOV);
    Rectangle xfovSliderRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*2), static_cast<float>(controlWidth), 20.0f};
    GuiSlider(xfovSliderRec, NULL, TextFormat("%d", config.XFOV), &xfov, 10.0f, 100.0f);
    config.XFOV = static_cast<int>(xfov);
    
    // YFOV slider
    Rectangle yfovLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*3), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(yfovLabelRec, "Y Field of View:");
    float yfov = static_cast<float>(config.YFOV);
    Rectangle yfovSliderRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*3), static_cast<float>(controlWidth), 20.0f};
    GuiSlider(yfovSliderRec, NULL, TextFormat("%d", config.YFOV), &yfov, 10.0f, 100.0f);
    config.YFOV = static_cast<int>(yfov);
    
    // NEW_FLICKSPEED slider
    Rectangle flickSpeedLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*4), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(flickSpeedLabelRec, "Flick Speed:");
    float flickSpeed = config.NEW_FLICKSPEED;
    Rectangle flickSpeedSliderRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*4), static_cast<float>(controlWidth), 20.0f};
    GuiSlider(flickSpeedSliderRec, NULL, TextFormat("%.1f", flickSpeed), &flickSpeed, 1.0f, 10.0f);
    config.NEW_FLICKSPEED = flickSpeed;
    
    // MOVESPEED slider
    Rectangle moveSpeedLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*5), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(moveSpeedLabelRec, "Move Speed:");
    float moveSpeed = config.MOVESPEED;
    Rectangle moveSpeedSliderRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*5), static_cast<float>(controlWidth), 20.0f};
    GuiSlider(moveSpeedSliderRec, NULL, TextFormat("%.2f", moveSpeed), &moveSpeed, 0.1f, 10.0f);
    config.MOVESPEED = moveSpeed;
}

void RayGUIConfig::DrawTriggerTab() {
    int startY = 85;
    int spacing = 30;
    int labelWidth = 150;
    int controlWidth = 120;
    int leftMargin = 20;
    
    // TOGGLE_TRIGGER key binding
    Rectangle toggleTriggerLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(toggleTriggerLabelRec, "Toggle Trigger Key:");
    OPAQUERECT toggleTriggerButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(toggleTriggerButtonRec, &config.TOGGLE_TRIGGER);
    
    // ACTIVATE_TRIGGER key binding
    Rectangle activateTriggerLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(activateTriggerLabelRec, "Activate Trigger Key:");
    OPAQUERECT activateTriggerButtonRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing), static_cast<float>(controlWidth), 20.0f};
    KeyBindButton(activateTriggerButtonRec, &config.ACTIVATE_TRIGGER);

    // WASD_SAFETY flag
    Rectangle wasdSafetyLabelRec = {static_cast<float>(leftMargin), static_cast<float>(startY + spacing*2), static_cast<float>(labelWidth), 20.0f};
    GuiLabel(wasdSafetyLabelRec, "WASD Safety:");
    Rectangle wasdSafetyCheckboxRec = {static_cast<float>(leftMargin + labelWidth), static_cast<float>(startY + spacing*2), static_cast<float>(controlWidth), 20.0f};
    GuiSetStyle(SLIDER, SLIDER_PADDING, 2);
    int sliderState = static_cast<int>(config.WASD_SAFETY);
    if (GuiToggleSlider(wasdSafetyCheckboxRec, "ON;OFF", &sliderState)) {
        config.WASD_SAFETY = !config.WASD_SAFETY;
    }
    GuiSetStyle(SLIDER, SLIDER_PADDING, 0);

}

void RayGUIConfig::CloseWindow() {
    // windowShouldClose flag always closes device handles
    if (DeviceConfig.activeHandle) {
        DeviceConfig.stopHandle();
    }

    ::CloseWindow();  // scope resolution operator resolves runtime polymorphism

    if (quitTargetchi) {
        exit(EXIT_SUCCESS);
    }
}

bool RayGUIConfig::KeyBindButton(OPAQUERECT b, int* keyValue) {
    bool pressed = false;

    Rectangle bounds = {
        b.x,
        b.y,
        b.width,
        b.height
    };

    // If this is the button we're waiting for key input on
    if (waitingForKeyInput && currentKeyBinding == keyValue) {
        // Draw the button with "Press any key..." text
        pressed = GuiButton(bounds, "Press any key...");
        
        // If clicked again, cancel the binding process
        if (pressed) {
            waitingForKeyInput = false;
            currentKeyBinding = nullptr;
        }
    } 
    else {
        // Draw normal button with the key name
        pressed = GuiButton(bounds, VirtualKeyToString(*keyValue));
        
        // If clicked, start waiting for key input
        if (pressed && !waitingForKeyInput) {
            waitingForKeyInput = true;
            currentKeyBinding = keyValue;
        }
    }
    
    return pressed;
}

const char* RayGUIConfig::VirtualKeyToString(int vKey) {
    auto it = keyNames.find(vKey);
    if (it != keyNames.end()) {
        return it->second;
    }
    return "Unknown";
}
