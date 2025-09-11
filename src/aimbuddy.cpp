#include "aimbuddy.h"
#include "show_gui.h"
#include "config_gui.h"
#include <iostream>
#include <Windows.h>

AimBuddy::AimBuddy(int x, int y, int xfov, int yfov, float movespeed,
             int activate_move_key, int activate_trigger_key, int color_mask) : 
        grabber(x, y, xfov, yfov), 
        movespeed(movespeed),
        activate_move_key(activate_move_key),
        activate_trigger_key(activate_trigger_key),
        toggled_move(false),
        toggled_trigger(false),
        gui_toggled(false),
        running(true),
        debug_screenshot(false), 
        frame_count(0), 
        color_mask(color_mask) {

    switch (color_mask) {
        case 0: // Purple
            lower_color = cv::Scalar(PURPLE_COLOR_MASK::LOWER_COLOR_H, 
                                     PURPLE_COLOR_MASK::LOWER_COLOR_S,
                                     PURPLE_COLOR_MASK::LOWER_COLOR_V);
            upper_color = cv::Scalar(PURPLE_COLOR_MASK::UPPER_COLOR_H,
                                     PURPLE_COLOR_MASK::UPPER_COLOR_S,
                                     PURPLE_COLOR_MASK::UPPER_COLOR_V);
            break;
        case 1: // Red
            lower_color = cv::Scalar(RED_COLOR_MASK::LOWER_COLOR_H,
                                     RED_COLOR_MASK::LOWER_COLOR_S,
                                     RED_COLOR_MASK::LOWER_COLOR_V);
            upper_color = cv::Scalar(RED_COLOR_MASK::UPPER_COLOR_H,
                                     RED_COLOR_MASK::UPPER_COLOR_S,
                                     RED_COLOR_MASK::UPPER_COLOR_V);
            break;
        case 2: // Yellow
            lower_color = cv::Scalar(YELLOW_COLOR_MASK::LOWER_COLOR_H,
                                     YELLOW_COLOR_MASK::LOWER_COLOR_S,
                                     YELLOW_COLOR_MASK::LOWER_COLOR_V);
            upper_color = cv::Scalar(YELLOW_COLOR_MASK::UPPER_COLOR_H,
                                     YELLOW_COLOR_MASK::UPPER_COLOR_S,
                                     YELLOW_COLOR_MASK::UPPER_COLOR_V);
            break;
    }
    
    start_time = std::chrono::steady_clock::now();

    // allocate CV operations
    // must be initialized the same as Capture::screen = cv::Mat(yfov, xfov, CV_8UC3); 
    screen = cv::Mat(yfov, xfov, CV_8UC3); 
    hsv.create(screen.size(), CV_8UC3);
    mask.create(screen.size(), CV_8UC1);
    dilated.create(screen.size(), CV_8UC1);
          
    // Start listening thread
    listen_thread = std::thread(&AimBuddy::listen, this);
}

AimBuddy::~AimBuddy() {
    close();
}

void AimBuddy::close() {
    running = false;
    if (listen_thread.joinable()) {
        listen_thread.join();
    }
}

void AimBuddy::toggle_move() {
    toggled_move = !toggled_move;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void AimBuddy::toggle_trigger() {
    toggled_trigger = !toggled_trigger;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void AimBuddy::show_fps() {
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

void AimBuddy::listen() {
    while (running) {
        bool m {(GetAsyncKeyState(activate_move_key) & 0x8000) && toggled_move};
        bool c {(GetAsyncKeyState(activate_trigger_key) & 0x8000) && toggled_trigger};

        if (m || c) {
            process(m, c);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));  // limit to 200 fps
    }
}

void AimBuddy::process(bool m, bool c) {
    std::lock_guard<std::mutex> lock(cv_mutex);
    //show_fps();

    screen = grabber.get_screen();

    cv::cvtColor(screen, hsv, cv::COLOR_BGR2HSV);
    
    cv::inRange(hsv, lower_color, upper_color, mask);
    
    // Dilate to improve contour detection
    cv::dilate(mask, dilated, cv::Mat(), cv::Point(-1, -1), 5);

    // if (!debug_screenshot) {
    //     cv::imwrite("C:\\Users\\arthu\\Desktop\\targetchi\\bin\\Debugcreenshot.png", dilated);
    //     debug_screenshot = true;
    // }

    // cv::imshow("Debug Capture", mask);
    // cv::waitKey(1);
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(dilated, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // If no contours found, return
    if (contours.empty()) {
        return;
    }
    
    // Find the largest contour
    size_t largest_idx = 0;
    double largest_area = 0;
    for (size_t i = 0; i < contours.size(); ++i) {
        double area = cv::contourArea(contours[i]);
        if (area > largest_area) {
            largest_area = area;
            largest_idx = i;
        }
    }
    
    // Get bounding rectangle of largest contour
    cv::Rect boundRect = cv::boundingRect(contours[largest_idx]);
    cv::Point center(boundRect.x + boundRect.width / 2, boundRect.y + boundRect.height / 2);
    int y_offset = static_cast<int>(boundRect.height * 0.3);
    
    if (m) {
        int cX = center.x;
        int cY = boundRect.y + y_offset;
        int x_diff = cX - grabber.xfov / 2;
        int y_diff = cY - grabber.yfov / 2;
        arduinomouse.set(static_cast<int>(x_diff * movespeed), static_cast<int>(y_diff * movespeed), c);
    } else if (c && 
             std::abs(center.x - grabber.xfov / 2) <= 4 && 
             std::abs(center.y - grabber.yfov / 2) <= 10) {
        arduinomouse.set(0, 0, c);
    }
}
