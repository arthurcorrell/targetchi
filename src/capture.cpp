#include "capture.h"
#include <iostream>

Capture::Capture(int x, int y, int xfov, int yfov) 
    : x(x), y(y), xfov(xfov), yfov(yfov), frame_count(0), running(true) {
    
    // Initialize screen with zeros (black image)
    screen = cv::Mat::zeros(yfov, xfov, CV_8UC3);
    
    // Start capture thread
    start_time = std::chrono::steady_clock::now();
    capture_thread = std::thread(&Capture::capture_loop, this);

    // https://learn.microsoft.com/en-us/windows/win32/gdi/capturing-an-image

    // device context handle for entire screen
    HDC hScreenDC = GetDC(NULL);
    // returns a handle to a memory DC
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    // create bitmap compatible with screen device context handle
    // it's color format matches color format of the hScreenDC device
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, xfov, yfov);

    // select the bitmap into the memory DC
    HBITMAP image = static_cast<HBITMAP>(SelectObject(hMemoryDC, hBitmap));

    // lpvBits: create buffer to store data
    screen = cv::Mat(yfov, xfov, CV_8UC3);

}

Capture::~Capture() {
    running = false;
    if (capture_thread.joinable()) {
        capture_thread.join();
    }
    SelectObject(hMemoryDC, image);
    DeleteObject(hBitmap);
    
    // called for CreateCompatibleDC
    DeleteDC(hMemoryDC);
    // called for GetDC
    ReleaseDC(NULL, hScreenDC);
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

    // performs bit block transfer- copies data from a src to dest bitmap
    // takes handles to 2 DCs, and copies bitmap selected into src DC to bitmap selected into dest DC
    // src: screenDC, dest: compatible DC
    // here: copies to hMemoryDC bitmap, a bitmap of xfov * yfov, from (x, y) of the hScreenDC bitmap 
    BitBlt(hMemoryDC, 0, 0, xfov, yfov, hScreenDC, x, y, SRCCOPY);

    // now image has been stored in memory. to redisplay, transfer from the dest DC to src DC

    // lpbmi: &BITMAPINFO struct, usage: bi.bmiColors = DIB_RGB_COLORS
    BITMAPINFO bmi = {0};
    // first member: BITMAPINFOHEADER bmiHeader;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = xfov;
    bmi.bmiHeader.biHeight = -yfov;  // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;  // 3 bytes per pixel (BGR)
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    // second member: RGBQUAD bmiColors[1]
    bmi.bmiColors[0].rgbBlue = 0;
    bmi.bmiColors[0].rgbGreen = 0;
    bmi.bmiColors[0].rgbRed = 0;
    bmi.bmiColors[0].rgbReserved = 0;

    
    // retrieve data from hBitmap, which is selected into hMemoryDC
    GetDIBits(hMemoryDC, hBitmap, 0, yfov, screen.data, &bmi, DIB_RGB_COLORS);

    
    
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
