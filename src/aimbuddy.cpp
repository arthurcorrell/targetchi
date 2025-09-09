#include "aimbuddy.h"
#include "show_gui.h"
#include "config_gui.h"
#include <iostream>
#include <Windows.h>

AimBuddy::AimBuddy(int x, int y, int xfov, int yfov, float flickspeed, float movespeed,
             int activate_move_key, int activate_trigger_key)
    : grabber(x, y, xfov, yfov), 
      flickspeed(flickspeed), 
      movespeed(movespeed),
      activate_move_key(activate_move_key),
      activate_trigger_key(activate_trigger_key),
      toggled_move(false),
      toggled_trigger(false),
      gui_toggled(false),
      running(true),
      debug_screenshot(false) {
          
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

void AimBuddy::listen() {
    while (running) {
        if ((GetAsyncKeyState(activate_move_key) & 0x8000) && toggled_move) {
            process("move");
        }
        else if ((GetAsyncKeyState(activate_trigger_key) & 0x8000) && toggled_trigger) {
            process("click");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));  // Reduce CPU usage
    }
}

void AimBuddy::process(const std::string& action) {
    cv::Mat screen = grabber.get_screen();
    cv::Mat hsv;
    cv::cvtColor(screen, hsv, cv::COLOR_BGR2HSV);
    
    // Create mask for color detection
    cv::Scalar lower_color(LOWER_COLOR_H, LOWER_COLOR_S, LOWER_COLOR_V);
    cv::Scalar upper_color(UPPER_COLOR_H, UPPER_COLOR_S, UPPER_COLOR_V);
    cv::Mat mask;
    cv::inRange(hsv, lower_color, upper_color, mask);
    
    // Dilate to improve contour detection
    cv::Mat dilated;
    cv::dilate(mask, dilated, cv::Mat(), cv::Point(-1, -1), 5);

    // if (!debug_screenshot) {
    //     cv::imwrite("C:\\Users\\arthu\\Desktop\\targetchi\\bin\\Debugcreenshot.png", dilated);
    //     debug_screenshot = true;
    // }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
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
    
    if (action == "move") {
        int cX = center.x;
        int cY = boundRect.y + y_offset;
        int x_diff = cX - grabber.xfov / 2;
        int y_diff = cY - grabber.yfov / 2;
        arduinomouse.move(static_cast<int>(x_diff * movespeed), static_cast<int>(y_diff * movespeed));
    }
    else if (action == "click" && 
             std::abs(center.x - grabber.xfov / 2) <= 4 && 
             std::abs(center.y - grabber.yfov / 2) <= 10) {
        arduinomouse.click();
    }
}
