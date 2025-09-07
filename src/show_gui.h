#pragma once

#include <functional>
#include <thread>
#include <atomic>

// Forward declaration
class AimBuddy;
class Capture;

void show_gui_window(Capture* grabber, std::function<bool()> gui_toggled);
void toggle_gui(AimBuddy* aimbuddy);
