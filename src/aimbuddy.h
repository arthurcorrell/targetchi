#pragma once

#include "capture.h"
#include "mouse.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <chrono>

// These constants are now managed by the config_gui.h
// and passed to the AimBuddy constructor

class AimBuddy {
private:
    ArduinoMouse arduinomouse;
    std::thread listen_thread;
    std::atomic<bool> running;
    int activate_move_key;
    int activate_trigger_key;
    bool wasd_safety;
    
    void listen();
    void process(const std::string& action);

public:
    Capture grabber;
    std::atomic<bool> toggled_move;
    std::atomic<bool> toggled_trigger;
    std::atomic<bool> gui_toggled;
    
    // Color thresholds for detection
    static constexpr int LOWER_COLOR_H = 140; // better than 135
    static constexpr int LOWER_COLOR_S = 120; // better than 10, filters more
    static constexpr int LOWER_COLOR_V = 173; // -> 173, old: 180
    static constexpr int UPPER_COLOR_H = 157; // -> 157, old: 160
    static constexpr int UPPER_COLOR_S = 255; // -> 255, old: 200
    static constexpr int UPPER_COLOR_V = 255; // -> 199 to remove KJ molly, old: 255
    
    float flickspeed;
    float movespeed;
    
    AimBuddy(int x, int y, int xfov, int yfov, float flickspeed, float movespeed,
            int activate_move_key = VK_RBUTTON, int activate_trigger_key = VK_LBUTTON);
    ~AimBuddy();
    
    void toggle_move();
    void toggle_trigger();
    void close();
    
    friend void toggle_gui(AimBuddy* aimbuddy);
};
