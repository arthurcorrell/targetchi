#pragma once

#include "capture.h"
#include "mouse.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
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
    bool debug_screenshot;
    int frame_count;
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    // these have unknown sizes, must be initialized the same as Capture::screen = cv::Mat(yfov, xfov, CV_8UC3); 
    cv::Mat screen;
    cv::Mat hsv;
    cv::Mat mask;
    cv::Mat dilated;
    
    void listen();
    void process(bool m, bool c);
    void show_fps();

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
    
    float movespeed;
    
    AimBuddy(int x, int y, int xfov, int yfov, float movespeed,
            int activate_move_key = VK_RBUTTON, int activate_trigger_key = VK_LBUTTON);
    ~AimBuddy();
    
    void toggle_move();
    void toggle_trigger();
    void close();
    
    friend void toggle_gui(AimBuddy* aimbuddy);
};
