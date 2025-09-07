#include "show_gui.h"
#include "aimbuddy.h"  // Include for full definition of AimBuddy

void show_gui_window(Capture* grabber, std::function<bool()> gui_toggled) {
    while (gui_toggled()) {
        // placeholder function for now. possibly will be used in future to 
        // open GUI for quick setting changes
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void toggle_gui(AimBuddy* aimbuddy) {
    aimbuddy->gui_toggled = !aimbuddy->gui_toggled;
    
    if (aimbuddy->gui_toggled) {
        std::thread gui_thread(show_gui_window, &aimbuddy->grabber, 
            [aimbuddy]() -> bool { return aimbuddy->gui_toggled; });
        gui_thread.detach();  // Detach thread to run independently
    }
}
