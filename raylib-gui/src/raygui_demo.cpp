#include "raylib.h"

// Define implementation of raygui
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// A minimal demo showing RayGUI usage without Windows API conflicts
// This can be extended to include all the functionality from config_raygui.cpp

// RayGUI Rectangle Test
int main(void) {
    // Initialize the window
    InitWindow(800, 600, "RayGUI Demo");
    SetTargetFPS(60);
    
    bool exitWindow = false;
    int currentTab = 0;
    char textBoxValue[64] = "Test";
    bool editMode = false;

    // Main game loop
    while (!WindowShouldClose() && !exitWindow) {
        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw tabs
        const char* groupText = "General;Aim;Trigger"; 
        
        // Create Rectangle struct properly (works in MSVC)
        Rectangle tabRect = { 10, 10, 320, 30 };
        currentTab = GuiToggleGroup(tabRect, groupText, &currentTab);
        
        // Draw save button
        Rectangle saveRect = { 700, 550, 80, 30 };
        if (GuiButton(saveRect, "Save")) {
            exitWindow = true;
        }
        
        // Draw text input
        Rectangle textRect = { 200, 100, 200, 30 };
        if (GuiTextBox(textRect, textBoxValue, 64, editMode)) {
            editMode = !editMode;
        }

        // Display tab index
        DrawText(TextFormat("Current Tab: %d", currentTab), 10, 100, 20, BLACK);
        
        EndDrawing();
    }
    
    // Close window and unload resources
    CloseWindow();
    
    return 0;
}
