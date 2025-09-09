#include "capture.h"
#include <iostream>

Capture::Capture(int x, int y, int xfov, int yfov) 
    : x(x), y(y), xfov(xfov), yfov(yfov), frame_count(0), running(true), debug_capture(false) {
    

    // https://learn.microsoft.com/en-us/windows/win32/gdi/capturing-an-image
    // used to be cv::Mat::zeros
    screen = cv::Mat(yfov, xfov, CV_8UC3); 

    // device context handle for entire screen
    hwnd = GetDesktopWindow();
    hScreenDC = GetDC(hwnd);
    // returns a handle to a memory DC
    hMemoryDC = CreateCompatibleDC(hScreenDC);
    SetStretchBltMode(hMemoryDC, COLORONCOLOR);
    // create bitmap compatible with screen device context handle
    // it's color format matches color format of the hScreenDC device
    hBitmap = CreateCompatibleBitmap(hScreenDC, xfov, yfov);

    // select the bitmap into the memory DC
    // HBITMAP image = static_cast<HBITMAP>();
    SelectObject(hMemoryDC, hBitmap);

    std::cout << "hScreenDC valid: " << (hScreenDC != NULL ? "yes" : "NO") << std::endl;
    std::cout << "hMemoryDC valid: " << (hMemoryDC != NULL ? "yes" : "NO") << std::endl;
    std::cout << "hBitmap valid: " << (hBitmap != NULL ? "yes" : "NO") << std::endl;
    
    // Start capture thread
    start_time = std::chrono::steady_clock::now();
    capture_thread = std::thread(&Capture::capture_loop, this);


}

Capture::~Capture() {
    running = false;
    if (capture_thread.joinable()) {
        capture_thread.join();
    }
    //SelectObject(hMemoryDC, image);
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
    //BitBlt(hMemoryDC, 0, 0, xfov, yfov, hScreenDC, x, y, SRCCOPY);
    // stretchBlt: allows for scaling of screen to second (xfov, yfov)
    StretchBlt(this->hMemoryDC, 0, 0, this->xfov, this->yfov, this->hScreenDC, this->x, this->y, this->xfov, this->yfov, SRCCOPY);
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

    // lpvBits: create buffer to store data
    // screen = cv::Mat(yfov, xfov, CV_8UC3);

    
    // retrieve data from hBitmap, which is selected into hMemoryDC
    GetDIBits(this->hMemoryDC, this->hBitmap, 0, this->yfov, this->screen.data, &bmi, DIB_RGB_COLORS);

    // Check pixel values at center
    int centerX = this->xfov / 2;
    int centerY = this->yfov / 2;
    cv::Vec3b centerPixel = this->screen.at<cv::Vec3b>(centerY, centerX);


    // cv::imshow("Debug Capture", this->screen);
    // cv::waitKey(1);

    // if (!debug_capture) {
    //     cv::imwrite("C:\\Users\\arthu\\Desktop\\targetchi\\bin\\debug_capture.png", screen);
    //     debug_capture = true;
    // }
    
    // Convert BGR to BGR (OpenCV default format is BGR)
    // This step might seem redundant, but it ensures format compatibility
    // cv::cvtColor(screen, screen, cv::COLOR_BGR2BGR);
}

void Capture::save_debug_frame() {
    static int frame_number = 0;
    std::string filename = "debug_frame_" + std::to_string(frame_number++) + ".png";
    cv::imwrite(filename, screen);
    std::cout << "Saved debug frame to " << filename << std::endl;
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
