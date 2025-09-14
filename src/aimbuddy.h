#pragma once

#include <Windows.h>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "mouse.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>

// These constants are now managed by the config_gui.h
// and passed to the AimBuddy constructor

struct PURPLE_COLOR_MASK {
    static constexpr int LOWER_COLOR_H = 140; // better than 135
    static constexpr int LOWER_COLOR_S = 120; // better than 10, filters more
    static constexpr int LOWER_COLOR_V = 173; // -> 173, old: 180
    static constexpr int UPPER_COLOR_H = 157; // -> 157, old: 160
    static constexpr int UPPER_COLOR_S = 255; // -> 255, old: 200
    static constexpr int UPPER_COLOR_V = 255; // -> 199 to remove KJ molly, old: 255
};

struct RED_COLOR_MASK {
    static constexpr int LOWER_COLOR_H = 0;
    static constexpr int LOWER_COLOR_S = 198;
    static constexpr int LOWER_COLOR_V = 243; 
    static constexpr int UPPER_COLOR_H = 0; 
    static constexpr int UPPER_COLOR_S = 255; 
    static constexpr int UPPER_COLOR_V = 255;
};

struct YELLOW_COLOR_MASK {
    static constexpr int LOWER_COLOR_H = 28;
    static constexpr int LOWER_COLOR_S = 75;
    static constexpr int LOWER_COLOR_V = 175; 
    static constexpr int UPPER_COLOR_H = 30; 
    static constexpr int UPPER_COLOR_S = 140; 
    static constexpr int UPPER_COLOR_V = 185;
};

class AimBuddy {
private:
    ArduinoMouse arduinomouse;
    std::thread listen_thread;
    std::atomic<bool> running;
    float movespeed;
    int activate_move_key;
    int activate_trigger_key;
    bool wasd_safety;
    int color_mask;

    cv::Scalar lower_color;
    cv::Scalar upper_color;

    // these have unknown sizes, must be initialized the same as Capture::screen = cv::Mat(yfov, xfov, CV_8UC3); 
    cv::Mat hsv;
    cv::Mat mask;
    cv::Mat dilated;
    
    void listen();
    void process(bool m, bool c);


    // capture code

    cv::Mat screen;
    std::mutex lock;
    
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int frame_count;
    
    void capture_loop();
    bool capture_screen();
    void update_fps();

    // DXGI screen duplication objects
    ID3D11Device* d3d_device = nullptr;
    ID3D11DeviceContext* d3d_context = nullptr;
    IDXGIOutputDuplication* dxgi_output_duplication = nullptr;
    
    // Screen texture resources
    ID3D11Texture2D* desktop_texture = nullptr;
    ID3D11Texture2D* staging_texture = nullptr;
    
    // Initialization methods
    bool init_dxgi_duplication();
    void release_dxgi_resources();

public:
    std::atomic<bool> toggled_move;
    std::atomic<bool> toggled_trigger;
    std::atomic<bool> gui_toggled;

    void* COLOR_MASK;
    
    AimBuddy(int x, int y, int xfov, int yfov, float movespeed,
            int activate_move_key, int activate_trigger_key, int color_mask);
    ~AimBuddy();
    
    void toggle_move();
    void toggle_trigger();
    void close();
    
    friend void toggle_gui(AimBuddy* aimbuddy);

    // capture 
    int x, y, xfov, yfov;
    int screenWidth, screenHeight;
};
