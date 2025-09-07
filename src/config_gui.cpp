// Use Unicode throughout the application
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include "config_gui.h"
#include "init.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <iomanip>


// Link with Comctl32.lib for modern controls
#pragma comment(lib, "comctl32.lib")

// Define the Ready2p font properties (approximation with system fonts)
#define FONT_HEIGHT 14
#define FONT_WIDTH 0
#define FONT_FAMILY L"MS Sans Serif"

// UI Constants
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 300;
constexpr int CONTROL_HEIGHT = 25;
constexpr int CONTROL_SPACING = 30;
constexpr int MARGIN = 20;
constexpr int TAB_HEIGHT = 30;
constexpr int LABEL_WIDTH = 150;
constexpr int CONTROL_WIDTH = 150;

// Macros for positioning
#define X_POS(col) (MARGIN + (col) * (LABEL_WIDTH + CONTROL_WIDTH + MARGIN))
#define Y_POS(row) (MARGIN + TAB_HEIGHT + (row) * CONTROL_SPACING)

// macros for positioning bounds checking
#define ELEM_SELECTED(row) ((x >= X_POS(0) + LABEL_WIDTH) && (x <= X_POS(0) + LABEL_WIDTH + CONTROL_WIDTH) && (y >= Y_POS(row)) && (y <= Y_POS(row) + CONTROL_HEIGHT))

// Class instance pointer for window procedure
ConfigGUI* g_configGUI = nullptr;

// Implementation of the ConfigGUI class
ConfigGUI::ConfigGUI(HINSTANCE hInstance, const AimbuddyConfig& initialConfig, ConfigCompletedCallback callback) 
    : hInstance_(hInstance), config_(initialConfig), callback_(callback), currentTab_(ID_TAB_GENERAL), hwnd_(NULL), activeTextboxId_(-1) {
    
    InitializeKeyMap();
    
    g_configGUI = this;
}

ConfigGUI::~ConfigGUI() {
    if (hFont_) DeleteObject(hFont_);
    if (hBackgroundBrush_) DeleteObject(hBackgroundBrush_);
    g_configGUI = nullptr;
}

bool ConfigGUI::Show() {
    // Register the window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.lpszClassName = L"AimbuddyConfigClass";


    
    if (!RegisterClassEx(&wc)) {
        LPCWSTR wrf = L"Window Registration Failed!";
        LPCWSTR err = L"Error";
        MessageBox(NULL, wrf, err, MB_ICONEXCLAMATION | MB_OK);
        return false;
    }
    
    // Create the window
    hwnd_ = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"AimbuddyConfigClass",
        L"Targetchi",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance_, NULL);
    
    if (hwnd_ == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }
    
    // Create the MacOS '91 style font
    hFont_ = CreateFont(FONT_HEIGHT, FONT_WIDTH, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FONT_FAMILY);
    
    // Create background brush (white)
    hBackgroundBrush_ = CreateSolidBrush(RGB(255, 255, 255));
    
    // Show the window
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return true;
}

LRESULT CALLBACK ConfigGUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_configGUI) {
        switch (msg) {
            /*
            OTHER USEFUL MESSAGES:
            WM_ACTIVATE: wParam

            */
            case WM_CREATE: {
                // Initialize controls here
                return 0;
            }
            
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Set the font
                SelectObject(hdc, g_configGUI->hFont_);
                SetBkMode(hdc, TRANSPARENT);
                
                // Fill background
                FillRect(hdc, &ps.rcPaint, g_configGUI->hBackgroundBrush_);
                
                // Draw tabs
                g_configGUI->DrawTab(hdc, ID_TAB_GENERAL, 10, 10, 100, TAB_HEIGHT, "General", 
                                    g_configGUI->currentTab_ == ID_TAB_GENERAL);
                g_configGUI->DrawTab(hdc, ID_TAB_AIM, 120, 10, 100, TAB_HEIGHT, "Aim", 
                                    g_configGUI->currentTab_ == ID_TAB_AIM);
                g_configGUI->DrawTab(hdc, ID_TAB_TRIGGER, 230, 10, 100, TAB_HEIGHT, "Trigger", 
                                    g_configGUI->currentTab_ == ID_TAB_TRIGGER);
                
                // Draw current tab content
                switch (g_configGUI->currentTab_) {
                    case ID_TAB_GENERAL:
                        g_configGUI->DrawGeneralTab(hdc);
                        break;
                    case ID_TAB_AIM:
                        g_configGUI->DrawAimTab(hdc);
                        break;
                    case ID_TAB_TRIGGER:
                        g_configGUI->DrawTriggerTab(hdc);
                        break;
                }
                
                // Draw save button
                g_configGUI->DrawButton(hdc, 1000, WINDOW_WIDTH - 120, WINDOW_HEIGHT - 100, 80, 30, "Save");
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            case WM_COMMAND:
                // Handle button clicks
                if (LOWORD(wParam) == 1000) { // Save button
                    g_configGUI->SaveAndClose();
                }
                return 0;
            
            case WM_LBUTTONDOWN:
                // Check if a tab was clicked
                {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);

                    if (g_configGUI->activeTextboxId_ != -1) {
                        g_configGUI->AssignKey(VK_LBUTTON);

                        // marks area for redrawing in WM_PAINT method 
                        InvalidateRect(hwnd, NULL, TRUE);
                        return 0;
                    }
                    
                    // Check general tab
                    if (x >= 10 && x <= 110 && y >= 10 && y <= 10 + TAB_HEIGHT) {
                        g_configGUI->currentTab_ = ID_TAB_GENERAL;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    // Check aim tab
                    else if (x >= 120 && x <= 220 && y >= 10 && y <= 10 + TAB_HEIGHT) {
                        g_configGUI->currentTab_ = ID_TAB_AIM;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    // Check trigger tab
                    else if (x >= 230 && x <= 330 && y >= 10 && y <= 10 + TAB_HEIGHT) {
                        g_configGUI->currentTab_ = ID_TAB_TRIGGER;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    // Check save button
                    else if (x >= WINDOW_WIDTH - 100 && x <= WINDOW_WIDTH - 20 && 
                             y >= WINDOW_HEIGHT - 100 && y <= WINDOW_HEIGHT - 20) {
                        g_configGUI->SaveAndClose();
                    }

                    // check listener fields  inside current tab
                    // starts listener for listener fields, writes to ConfigGUI::_config

                    switch (g_configGUI->currentTab_) {
                        case ID_TAB_GENERAL:
                            if (ELEM_SELECTED(0)) {
                                g_configGUI->activeTextboxId_ = 1; // quit key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } else if (ELEM_SELECTED(1)) {
                                g_configGUI->activeTextboxId_ = 2; // toggle key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } 
                            break;
                        case ID_TAB_AIM:
                            if (ELEM_SELECTED(0)) {
                                g_configGUI->activeTextboxId_ = 4; // toggle move key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } else if (ELEM_SELECTED(1)) {
                                g_configGUI->activeTextboxId_ = 5; // activate move key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } 
                            break;
                        case ID_TAB_TRIGGER:
                            if (ELEM_SELECTED(0)) {
                                g_configGUI->activeTextboxId_ = 10; // toggle trigger key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } else if (ELEM_SELECTED(1)) {
                                g_configGUI->activeTextboxId_ = 11; // activate trigger key
                                InvalidateRect(hwnd, NULL, TRUE);
                            } 
                            break;
                    }
                }
                return 0;
            
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            /*
            posted to keyboard focus window when key is pressed without ALT key
            */
            {
                if (g_configGUI->activeTextboxId_ != -1) {
                    int vk = static_cast<int>(wParam);
                    g_configGUI->AssignKey(vk);

                    // marks area for redrawing in WM_PAINT method 
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            }
            break;

            case WM_MBUTTONDOWN:
                if (g_configGUI->activeTextboxId_ != -1) {
                    g_configGUI->AssignKey(VK_MBUTTON);

                    // marks area for redrawing in WM_PAINT method 
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            break;

            case WM_RBUTTONDOWN:
                if (g_configGUI->activeTextboxId_ != -1) {
                    g_configGUI->AssignKey(VK_RBUTTON);

                    // marks area for redrawing in WM_PAINT method 
                    InvalidateRect(hwnd, NULL, TRUE);
                    return 0;
                }
            break;

            case WM_XBUTTONDOWN:
            {
                if (g_configGUI->activeTextboxId_ != -1) {
                    // Check which X button was pressed (side button)
                    UINT button = HIWORD(wParam);
                    int vk = (button == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
                    g_configGUI->AssignKey(vk);
        
                    InvalidateRect(hwnd, NULL, TRUE);
                    return TRUE; // Must return TRUE for WM_XBUTTONDOWN
                }
            }
            break;

            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
                
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ConfigGUI::AssignKey(int vk) {
    switch (activeTextboxId_) {
        case 1: 
            config_.QUIT = vk;
            break;
        case 2:
            config_.TOGGLE_KEY = vk;
            break;
        case 4:
            config_.TOGGLE_MOVE = vk;
            break;
        case 5:
            config_.ACTIVATE_MOVE = vk;
            break;
        case 10:
            config_.TOGGLE_TRIGGER = vk;
            break;
        case 11:
            config_.ACTIVATE_TRIGGER = vk;
            break;
    }
    activeTextboxId_ = -1;  // Reset active textbox
}

void ConfigGUI::DrawGeneralTab(HDC hdc) {
    // Draw labels and input fields for General settings
    DrawLabel(hdc, X_POS(0), Y_POS(0), "Quit Key:");
    DrawTab(hdc, 1, X_POS(0) + LABEL_WIDTH, Y_POS(0), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.QUIT), activeTextboxId_ == 1);
    
    DrawLabel(hdc, X_POS(0), Y_POS(1), "Toggle Key:");
    DrawTab(hdc, 2, X_POS(0) + LABEL_WIDTH, Y_POS(1), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.TOGGLE_KEY), activeTextboxId_ == 2);
    
    DrawLabel(hdc, X_POS(0), Y_POS(2), "In-game Sensitivity:");
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << config_.INGAME_SENSITIVITY;
    DrawTextbox(hdc, 3, X_POS(0) + LABEL_WIDTH, Y_POS(2), CONTROL_WIDTH, ss.str());
}

void ConfigGUI::DrawAimTab(HDC hdc) {
    // Draw labels and input fields for Aim settings
    DrawLabel(hdc, X_POS(0), Y_POS(0), "Toggle Aim Key:");
    DrawTab(hdc, 4, X_POS(0) + LABEL_WIDTH, Y_POS(0), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.TOGGLE_MOVE), activeTextboxId_ == 4);
    
    DrawLabel(hdc, X_POS(0), Y_POS(1), "Activate Aim Key:");
    DrawTab(hdc, 5, X_POS(0) + LABEL_WIDTH, Y_POS(1), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.ACTIVATE_MOVE), activeTextboxId_ == 5);
    
    DrawLabel(hdc, X_POS(0), Y_POS(2), "X FOV:");
    std::stringstream ss1;
    ss1 << config_.XFOV;
    DrawTextbox(hdc, 6, X_POS(0) + LABEL_WIDTH, Y_POS(2), CONTROL_WIDTH, ss1.str());
    
    DrawLabel(hdc, X_POS(0), Y_POS(3), "Y FOV:");
    std::stringstream ss2;
    ss2 << config_.YFOV;
    DrawTextbox(hdc, 7, X_POS(0) + LABEL_WIDTH, Y_POS(3), CONTROL_WIDTH, ss2.str());
    
    DrawLabel(hdc, X_POS(0), Y_POS(4), "Move Speed:");
    std::stringstream ss3;
    ss3 << std::fixed << std::setprecision(2) << config_.MOVESPEED;
    DrawTextbox(hdc, 8, X_POS(0) + LABEL_WIDTH, Y_POS(4), CONTROL_WIDTH, ss3.str());
}

void ConfigGUI::DrawTriggerTab(HDC hdc) {
    // Draw labels and input fields for Trigger settings
    DrawLabel(hdc, X_POS(0), Y_POS(0), "Toggle Trigger Key:");
    DrawTab(hdc, 10, X_POS(0) + LABEL_WIDTH, Y_POS(0), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.TOGGLE_TRIGGER), activeTextboxId_ == 10);

    DrawLabel(hdc, X_POS(0), Y_POS(1), "Activate Trigger Key:");
    DrawTab(hdc, 11, X_POS(0) + LABEL_WIDTH, Y_POS(1), CONTROL_WIDTH, CONTROL_HEIGHT, VKToKeyName(config_.ACTIVATE_TRIGGER), activeTextboxId_ == 11);
}

void ConfigGUI::DrawLabel(HDC hdc, int x, int y, const std::string& text) {
    std::wstring wtext = StringToWString(text);
    TextOutW(hdc, x, y, wtext.c_str(), (int)wtext.length());
}

void ConfigGUI::DrawTextbox(HDC hdc, int id, int x, int y, int width, const std::string& text) {
    // Draw a simple MacOS '91 style textbox
    RECT rect = { x, y, x + width, y + CONTROL_HEIGHT };
    DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT);
    InflateRect(&rect, -3, -3);
    std::wstring wtext = StringToWString(text);
    DrawTextW(hdc, wtext.c_str(), -1, &rect, DT_VCENTER | DT_LEFT);
}

void ConfigGUI::DrawButton(HDC hdc, int id, int x, int y, int width, int height, const std::string& text) {
    // Draw a simple MacOS '91 style button
    RECT rect = { x, y, x + width, y + height };
    DrawEdge(hdc, &rect, EDGE_RAISED, BF_RECT);
    
    // Center text in button
    std::wstring wtext = StringToWString(text);
    SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
    TextOutW(hdc, x + width / 2, y + height / 2 + 5, wtext.c_str(), (int)wtext.length());
    SetTextAlign(hdc, TA_LEFT | TA_TOP);
}

void ConfigGUI::DrawTab(HDC hdc, int id, int x, int y, int width, int height, const std::string& text, bool selected) {
    // Draw a MacOS '91 style tab
    RECT rect = { x, y, x + width, y + height };
    
    if (selected) {
        // Selected tab is white
        FillRect(hdc, &rect, hBackgroundBrush_);
        // Draw a black line under all tabs except the selected one
        MoveToEx(hdc, 10, y + height, NULL);
        LineTo(hdc, WINDOW_WIDTH - 10, y + height);
        // Draw black border only on top and sides
        MoveToEx(hdc, x, y + height, NULL);
        LineTo(hdc, x, y);
        LineTo(hdc, x + width, y);
        LineTo(hdc, x + width, y + height);
    } else {
        // Unselected tabs have a light gray background
        HBRUSH grayBrush = CreateSolidBrush(RGB(220, 220, 220));
        FillRect(hdc, &rect, grayBrush);
        DeleteObject(grayBrush);
        
        // Draw border
        DrawEdge(hdc, &rect, EDGE_RAISED, BF_RECT);
    }
    
    // Center text in tab
    std::wstring wtext = StringToWString(text);
    SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
    TextOutW(hdc, x + width / 2, y + height / 2 + 5, wtext.c_str(), (int)wtext.length());
    SetTextAlign(hdc, TA_LEFT | TA_TOP);
}

void ConfigGUI::SaveAndClose() {
    // Call the callback with the updated config
    callback_(config_);
    
    // Close the window
    SendMessage(hwnd_, WM_CLOSE, 0, 0);
}

int ConfigGUI::KeyNameToVK(const std::string& keyName) {
    auto it = keyMap_.find(keyName);
    if (it != keyMap_.end()) {
        return it->second;
    }
    return 0; // Default value if not found
}

std::string ConfigGUI::VKToKeyName(int vk) {
    for (const auto& pair : keyMap_) {
        if (pair.second == vk) {
            return pair.first;
        }
    }
    
    // Handle numbers and letters
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        return std::string(1, (char)vk);
    }
    
    return "UNKNOWN"; // Default if not found
}

// Function to show config dialog and get settings
AimbuddyConfig ShowConfigDialog(HINSTANCE hInstance, const AimbuddyConfig& initialConfig) {
    AimbuddyConfig resultConfig = initialConfig;
    
    // Create a modal dialog
    ConfigGUI configGUI(hInstance, initialConfig, [&resultConfig](const AimbuddyConfig& config) {
        resultConfig = config;
    });
    
    configGUI.Show();
    
    return resultConfig;
}

void ConfigGUI::setMouseType(MouseType mousetype) {
    BYTE reportBuffer[static_cast<BYTE>(7)] = {
        static_cast<BYTE>(2),
        static_cast<BYTE>(mousetype),
        0x00,
        0x00,
        0x00,
        0x00
    };

    // Send HID output report
    // if (!HidD_SetOutputReport(dev, reportBuffer, sizeof(reportBuffer))) {
    //     std::cerr << "Failed to send HID report" << std::endl;
    // }

    DWORD bytesWritten = 0;
    OVERLAPPED overlapped = {0};
    
    // Use WriteFile for sending to the interrupt endpoint. returns 0 when async
    WriteFile(dev, reportBuffer, sizeof(reportBuffer), &bytesWritten, &overlapped);
}

// keymap for GUI display
void ConfigGUI::InitializeKeyMap() {
    // Mouse buttons
    keyMap_["LBUTTON"] = VK_LBUTTON;       // 0x01 - Left mouse button
    keyMap_["RBUTTON"] = VK_RBUTTON;       // 0x02 - Right mouse button
    keyMap_["MBUTTON"] = VK_MBUTTON;       // 0x04 - Middle mouse button
    keyMap_["XBUTTON1"] = VK_XBUTTON1;     // 0x05 - X1 mouse button
    keyMap_["XBUTTON2"] = VK_XBUTTON2;     // 0x06 - X2 mouse button
    
    // Basic control keys
    keyMap_["BACK"] = VK_BACK;             // 0x08 - Backspace key
    keyMap_["TAB"] = VK_TAB;               // 0x09 - Tab key
    keyMap_["CLEAR"] = VK_CLEAR;           // 0x0C - Clear key
    keyMap_["RETURN"] = VK_RETURN;         // 0x0D - Enter key
    keyMap_["ENTER"] = VK_RETURN;          // 0x0D - Enter key (alias)
    
    // Modifier keys
    keyMap_["SHIFT"] = VK_SHIFT;           // 0x10 - Shift key
    keyMap_["CONTROL"] = VK_CONTROL;       // 0x11 - Ctrl key
    keyMap_["CTRL"] = VK_CONTROL;          // 0x11 - Ctrl key (alias)
    keyMap_["ALT"] = VK_MENU;              // 0x12 - Alt key
    keyMap_["MENU"] = VK_MENU;             // 0x12 - Alt key (original name)
    keyMap_["PAUSE"] = VK_PAUSE;           // 0x13 - Pause key
    keyMap_["CAPITAL"] = VK_CAPITAL;       // 0x14 - Caps lock key
    keyMap_["CAPSLOCK"] = VK_CAPITAL;      // 0x14 - Caps lock key (alias)
    
    // Common control keys
    keyMap_["ESC"] = VK_ESCAPE;            // 0x1B - Esc key
    keyMap_["ESCAPE"] = VK_ESCAPE;         // 0x1B - Esc key (alias)
    keyMap_["SPACE"] = VK_SPACE;           // 0x20 - Spacebar
    
    // Navigation keys
    keyMap_["PGUP"] = VK_PRIOR;            // 0x21 - Page up key
    keyMap_["PRIOR"] = VK_PRIOR;           // 0x21 - Page up key (original name)
    keyMap_["PGDN"] = VK_NEXT;             // 0x22 - Page down key
    keyMap_["NEXT"] = VK_NEXT;             // 0x22 - Page down key (original name)
    keyMap_["END"] = VK_END;               // 0x23 - End key
    keyMap_["HOME"] = VK_HOME;             // 0x24 - Home key
    keyMap_["LEFT"] = VK_LEFT;             // 0x25 - Left arrow
    keyMap_["UP"] = VK_UP;                 // 0x26 - Up arrow
    keyMap_["RIGHT"] = VK_RIGHT;           // 0x27 - Right arrow
    keyMap_["DOWN"] = VK_DOWN;             // 0x28 - Down arrow
    
    // Misc control keys
    keyMap_["SELECT"] = VK_SELECT;         // 0x29 - Select key
    keyMap_["PRINT"] = VK_PRINT;           // 0x2A - Print key
    keyMap_["EXECUTE"] = VK_EXECUTE;       // 0x2B - Execute key
    keyMap_["SNAPSHOT"] = VK_SNAPSHOT;     // 0x2C - Print screen key
    keyMap_["PRTSC"] = VK_SNAPSHOT;        // 0x2C - Print screen key (alias)
    keyMap_["INSERT"] = VK_INSERT;         // 0x2D - Insert key
    keyMap_["INS"] = VK_INSERT;            // 0x2D - Insert key (alias)
    keyMap_["DELETE"] = VK_DELETE;         // 0x2E - Delete key
    keyMap_["DEL"] = VK_DELETE;            // 0x2E - Delete key (alias)
    keyMap_["HELP"] = VK_HELP;             // 0x2F - Help key
    
    // Number keys
    keyMap_["0"] = 0x30;                   // 0x30 - 0 key
    keyMap_["1"] = 0x31;                   // 0x31 - 1 key
    keyMap_["2"] = 0x32;                   // 0x32 - 2 key
    keyMap_["3"] = 0x33;                   // 0x33 - 3 key
    keyMap_["4"] = 0x34;                   // 0x34 - 4 key
    keyMap_["5"] = 0x35;                   // 0x35 - 5 key
    keyMap_["6"] = 0x36;                   // 0x36 - 6 key
    keyMap_["7"] = 0x37;                   // 0x37 - 7 key
    keyMap_["8"] = 0x38;                   // 0x38 - 8 key
    keyMap_["9"] = 0x39;                   // 0x39 - 9 key
    
    // Letter keys
    keyMap_["A"] = 0x41;                   // 0x41 - A key
    keyMap_["B"] = 0x42;                   // 0x42 - B key
    keyMap_["C"] = 0x43;                   // 0x43 - C key
    keyMap_["D"] = 0x44;                   // 0x44 - D key
    keyMap_["E"] = 0x45;                   // 0x45 - E key
    keyMap_["F"] = 0x46;                   // 0x46 - F key
    keyMap_["G"] = 0x47;                   // 0x47 - G key
    keyMap_["H"] = 0x48;                   // 0x48 - H key
    keyMap_["I"] = 0x49;                   // 0x49 - I key
    keyMap_["J"] = 0x4A;                   // 0x4A - J key
    keyMap_["K"] = 0x4B;                   // 0x4B - K key
    keyMap_["L"] = 0x4C;                   // 0x4C - L key
    keyMap_["M"] = 0x4D;                   // 0x4D - M key
    keyMap_["N"] = 0x4E;                   // 0x4E - N key
    keyMap_["O"] = 0x4F;                   // 0x4F - O key
    keyMap_["P"] = 0x50;                   // 0x50 - P key
    keyMap_["Q"] = 0x51;                   // 0x51 - Q key
    keyMap_["R"] = 0x52;                   // 0x52 - R key
    keyMap_["S"] = 0x53;                   // 0x53 - S key
    keyMap_["T"] = 0x54;                   // 0x54 - T key
    keyMap_["U"] = 0x55;                   // 0x55 - U key
    keyMap_["V"] = 0x56;                   // 0x56 - V key
    keyMap_["W"] = 0x57;                   // 0x57 - W key
    keyMap_["X"] = 0x58;                   // 0x58 - X key
    keyMap_["Y"] = 0x59;                   // 0x59 - Y key
    keyMap_["Z"] = 0x5A;                   // 0x5A - Z key
    
    // Windows keys
    keyMap_["LWIN"] = VK_LWIN;             // 0x5B - Left Windows key
    keyMap_["RWIN"] = VK_RWIN;             // 0x5C - Right Windows key
    keyMap_["APPS"] = VK_APPS;             // 0x5D - Applications key
    keyMap_["SLEEP"] = VK_SLEEP;           // 0x5F - Sleep key
    
    // Numpad keys
    keyMap_["NUMPAD0"] = VK_NUMPAD0;       // 0x60 - Numpad 0
    keyMap_["NUMPAD1"] = VK_NUMPAD1;       // 0x61 - Numpad 1
    keyMap_["NUMPAD2"] = VK_NUMPAD2;       // 0x62 - Numpad 2
    keyMap_["NUMPAD3"] = VK_NUMPAD3;       // 0x63 - Numpad 3
    keyMap_["NUMPAD4"] = VK_NUMPAD4;       // 0x64 - Numpad 4
    keyMap_["NUMPAD5"] = VK_NUMPAD5;       // 0x65 - Numpad 5
    keyMap_["NUMPAD6"] = VK_NUMPAD6;       // 0x66 - Numpad 6
    keyMap_["NUMPAD7"] = VK_NUMPAD7;       // 0x67 - Numpad 7
    keyMap_["NUMPAD8"] = VK_NUMPAD8;       // 0x68 - Numpad 8
    keyMap_["NUMPAD9"] = VK_NUMPAD9;       // 0x69 - Numpad 9
    keyMap_["MULTIPLY"] = VK_MULTIPLY;     // 0x6A - Multiply key
    keyMap_["ADD"] = VK_ADD;               // 0x6B - Add key
    keyMap_["SEPARATOR"] = VK_SEPARATOR;   // 0x6C - Separator key
    keyMap_["SUBTRACT"] = VK_SUBTRACT;     // 0x6D - Subtract key
    keyMap_["DECIMAL"] = VK_DECIMAL;       // 0x6E - Decimal key
    keyMap_["DIVIDE"] = VK_DIVIDE;         // 0x6F - Divide key
    
    // Function keys
    keyMap_["F1"] = VK_F1;                 // 0x70 - F1 key
    keyMap_["F2"] = VK_F2;                 // 0x71 - F2 key
    keyMap_["F3"] = VK_F3;                 // 0x72 - F3 key
    keyMap_["F4"] = VK_F4;                 // 0x73 - F4 key
    keyMap_["F5"] = VK_F5;                 // 0x74 - F5 key
    keyMap_["F6"] = VK_F6;                 // 0x75 - F6 key
    keyMap_["F7"] = VK_F7;                 // 0x76 - F7 key
    keyMap_["F8"] = VK_F8;                 // 0x77 - F8 key
    keyMap_["F9"] = VK_F9;                 // 0x78 - F9 key
    keyMap_["F10"] = VK_F10;               // 0x79 - F10 key
    keyMap_["F11"] = VK_F11;               // 0x7A - F11 key
    keyMap_["F12"] = VK_F12;               // 0x7B - F12 key
    keyMap_["F13"] = VK_F13;               // 0x7C - F13 key
    keyMap_["F14"] = VK_F14;               // 0x7D - F14 key
    keyMap_["F15"] = VK_F15;               // 0x7E - F15 key
    keyMap_["F16"] = VK_F16;               // 0x7F - F16 key
    keyMap_["F17"] = VK_F17;               // 0x80 - F17 key
    keyMap_["F18"] = VK_F18;               // 0x81 - F18 key
    keyMap_["F19"] = VK_F19;               // 0x82 - F19 key
    keyMap_["F20"] = VK_F20;               // 0x83 - F20 key
    keyMap_["F21"] = VK_F21;               // 0x84 - F21 key
    keyMap_["F22"] = VK_F22;               // 0x85 - F22 key
    keyMap_["F23"] = VK_F23;               // 0x86 - F23 key
    keyMap_["F24"] = VK_F24;               // 0x87 - F24 key
    
    // Modifier keys (specific)
    keyMap_["LSHIFT"] = VK_LSHIFT;         // 0xA0 - Left shift key
    keyMap_["RSHIFT"] = VK_RSHIFT;         // 0xA1 - Right shift key
    keyMap_["LCONTROL"] = VK_LCONTROL;     // 0xA2 - Left control key
    keyMap_["LCTRL"] = VK_LCONTROL;        // 0xA2 - Left control key (alias)
    keyMap_["RCONTROL"] = VK_RCONTROL;     // 0xA3 - Right control key
    keyMap_["RCTRL"] = VK_RCONTROL;        // 0xA3 - Right control key (alias)
    keyMap_["LMENU"] = VK_LMENU;           // 0xA4 - Left alt key
    keyMap_["LALT"] = VK_LMENU;            // 0xA4 - Left alt key (alias)
    keyMap_["RMENU"] = VK_RMENU;           // 0xA5 - Right alt key
    keyMap_["RALT"] = VK_RMENU;            // 0xA5 - Right alt key (alias)
    
    // Browser keys
    keyMap_["BROWSER_BACK"] = VK_BROWSER_BACK;           // 0xA6
    keyMap_["BROWSER_FORWARD"] = VK_BROWSER_FORWARD;     // 0xA7
    keyMap_["BROWSER_REFRESH"] = VK_BROWSER_REFRESH;     // 0xA8
    keyMap_["BROWSER_STOP"] = VK_BROWSER_STOP;           // 0xA9
    keyMap_["BROWSER_SEARCH"] = VK_BROWSER_SEARCH;       // 0xAA
    keyMap_["BROWSER_FAVORITES"] = VK_BROWSER_FAVORITES; // 0xAB
    keyMap_["BROWSER_HOME"] = VK_BROWSER_HOME;           // 0xAC
    
    // OEM keys
    keyMap_["OEM_1"] = VK_OEM_1;           // 0xBA - ;: key for US
    keyMap_["SEMICOLON"] = VK_OEM_1;       // 0xBA - ;: key for US (alias)
    keyMap_["OEM_PLUS"] = VK_OEM_PLUS;     // 0xBB - +- key for any country
    keyMap_["PLUS"] = VK_OEM_PLUS;         // 0xBB - +- key for any country (alias)
    keyMap_["OEM_COMMA"] = VK_OEM_COMMA;   // 0xBC - ,< key for any country
    keyMap_["COMMA"] = VK_OEM_COMMA;       // 0xBC - ,< key for any country (alias)
    keyMap_["OEM_MINUS"] = VK_OEM_MINUS;   // 0xBD - -_ key for any country
    keyMap_["MINUS"] = VK_OEM_MINUS;       // 0xBD - -_ key for any country (alias)
    keyMap_["OEM_PERIOD"] = VK_OEM_PERIOD; // 0xBE - .> key for any country
    keyMap_["PERIOD"] = VK_OEM_PERIOD;     // 0xBE - .> key for any country (alias)
    keyMap_["OEM_2"] = VK_OEM_2;           // 0xBF - /? key for US
    keyMap_["SLASH"] = VK_OEM_2;           // 0xBF - /? key for US (alias)
    keyMap_["OEM_3"] = VK_OEM_3;           // 0xC0 - `~ key for US
    keyMap_["TILDE"] = VK_OEM_3;           // 0xC0 - `~ key for US (alias)
    
    keyMap_["OEM_4"] = VK_OEM_4;           // 0xDB - [{ key for US
    keyMap_["LBRACKET"] = VK_OEM_4;        // 0xDB - [{ key for US (alias)
    keyMap_["OEM_5"] = VK_OEM_5;           // 0xDC - \| key for US
    keyMap_["BACKSLASH"] = VK_OEM_5;       // 0xDC - \| key for US (alias)
    keyMap_["OEM_6"] = VK_OEM_6;           // 0xDD - ]} key for US
    keyMap_["RBRACKET"] = VK_OEM_6;        // 0xDD - ]} key for US (alias)
    keyMap_["OEM_7"] = VK_OEM_7;           // 0xDE - '" key for US
    keyMap_["QUOTE"] = VK_OEM_7;           // 0xDE - '" key for US (alias)
    keyMap_["OEM_8"] = VK_OEM_8;           // 0xDF - Miscellaneous
    
    keyMap_["OEM_102"] = VK_OEM_102;       // 0xE2 - \| key on RT 102-key keyboard

    
    // Attn, etc. keys
    keyMap_["ATTN"] = VK_ATTN;             // 0xF6
    keyMap_["CRSEL"] = VK_CRSEL;           // 0xF7
    keyMap_["EXSEL"] = VK_EXSEL;           // 0xF8
    keyMap_["EREOF"] = VK_EREOF;           // 0xF9
    keyMap_["PLAY"] = VK_PLAY;             // 0xFA
    keyMap_["ZOOM"] = VK_ZOOM;             // 0xFB
    keyMap_["PA1"] = VK_PA1;               // 0xFD
    keyMap_["OEM_CLEAR"] = VK_OEM_CLEAR;   // 0xFE
}
