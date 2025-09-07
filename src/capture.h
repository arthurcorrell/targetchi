#pragma once

#include <Windows.h>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

// OpenCV headers
#include <opencv2/opencv.hpp>

class Capture {
private:
    cv::Mat screen;
    cv::Mat image;
    std::mutex lock;
    std::thread capture_thread;
    std::atomic<bool> running;
    
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int frame_count;
    
    void capture_loop();
    void capture_screen();
    void update_fps();

public:
    Capture(int x, int y, int xfov, int yfov);
    ~Capture();
    int x, y, xfov, yfov;
    
    cv::Mat get_screen();
};
