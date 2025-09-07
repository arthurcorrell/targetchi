#include "capture.h"
#include <iostream>

Capture::Capture(int x, int y, int xfov, int yfov) 
    : x(x), y(y), xfov(xfov), yfov(yfov), frame_count(0), running(true) {
    
    // Initialize screen with zeros (black image)
    screen = cv::Mat::zeros(yfov, xfov, CV_8UC3);
    
    // Start capture thread
    start_time = std::chrono::steady_clock::now();
    capture_thread = std::thread(&Capture::capture_loop, this);
}

Capture::~Capture() {
    running = false;
    if (capture_thread.joinable()) {
        capture_thread.join();
    }
}

void Capture::capture_loop() {
    while (running) {
        {
            std::lock_guard<std::mutex> guard(lock);
            capture_screen();
        }
        update_fps();
        
        // Small sleep to avoid consuming too much CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Capture::capture_screen() {
    // Create compatible DC for screen
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    
    // Create compatible bitmap
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, xfov, yfov);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);
    
    // Copy screen to bitmap
    BitBlt(hMemoryDC, 0, 0, xfov, yfov, hScreenDC, x, y, SRCCOPY);
    
    // Get bitmap info
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = xfov;
    bi.biHeight = -yfov;  // Negative height to start from top-left
    bi.biPlanes = 1;
    bi.biBitCount = 24;  // 3 bytes (BGR)
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    
    // Create OpenCV Mat
    screen = cv::Mat(yfov, xfov, CV_8UC3);
    
    // Get bitmap data
    GetDIBits(hMemoryDC, hBitmap, 0, yfov, screen.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    
    // Clean up
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
    
    // Convert BGR to BGR (OpenCV default format is BGR)
    // This step might seem redundant, but it ensures format compatibility
    // cv::cvtColor(screen, screen, cv::COLOR_BGR2BGR);
}

void Capture::update_fps() {
    frame_count++;
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
    
    if (elapsed_time >= 1) {
        double fps = frame_count / static_cast<double>(elapsed_time);
        std::cout << " - FPS: " << static_cast<int>(fps) << "\r" << std::flush;
        frame_count = 0;
        start_time = current_time;
    }
}

cv::Mat Capture::get_screen() {
    std::lock_guard<std::mutex> guard(lock);
    return screen.clone();
}
