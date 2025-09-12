#pragma once

#include <Windows.h>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <d3d11.h>
#include <dxgi1_2.h>

// OpenCV headers
#include <opencv2/opencv.hpp>

class Capture {
private:
    cv::Mat screen;
    std::mutex lock;
    std::thread capture_thread;
    std::atomic<bool> running;
    
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int frame_count;
    bool debug_capture;
    
    void capture_loop();
    void capture_screen();
    void update_fps();
    void save_debug_frame();

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
    Capture(int x, int y, int xfov, int yfov);
    ~Capture();
    int x, y, xfov, yfov;
    int screenWidth, screenHeight;
    
    cv::Mat get_screen();
};
