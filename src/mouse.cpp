#include "mouse.h"
#include "../raylib-gui/include/init.h"
#include <iostream>

ArduinoMouse::ArduinoMouse(size_t filter_length) 
    : filter_length(filter_length), gen(rd()), dis(0.01, 0.1) {
    
    x_history.resize(filter_length, 0);
    y_history.resize(filter_length, 0);

    DeviceConfig.startHandle();

}

ArduinoMouse::~ArduinoMouse() {
    close();
}

// set output report bits. delay must be computed on device
void ArduinoMouse::set(int x, int y, bool b) {
    if ((x != 0) || (y != 0)) {
        // add history vals only on valid move calls, x=0 && y=0 edge cases are lost
        x_history.push_back(x);
        y_history.push_back(y);
        
        x_history.erase(x_history.begin());
        y_history.erase(y_history.begin());
        
        int smooth_x = std::accumulate(x_history.begin(), x_history.end(), 0) / filter_length;
        int smooth_y = std::accumulate(y_history.begin(), y_history.end(), 0) / filter_length;

        // ADD MOVESPEED MULTIPLIER PROPORTIONAL TO DISTANCE WITH MIN CLAMP AT 1

        DeviceConfig.setOutputReport(smooth_x, smooth_y, static_cast<int>(b));
    } else {
        // static cast for edge case where x, y is computed to 0
        DeviceConfig.setOutputReport(0, 0, static_cast<int>(b));
    }
}

// antiquated
void ArduinoMouse::move(int x, int y) {
    // Add new values to history
    x_history.push_back(x);
    y_history.push_back(y);
    
    // Remove oldest values
    x_history.erase(x_history.begin());
    y_history.erase(y_history.begin());
    
    // Calculate averages
    int smooth_x = std::accumulate(x_history.begin(), x_history.end(), 0) / filter_length;
    int smooth_y = std::accumulate(y_history.begin(), y_history.end(), 0) / filter_length;
    
    DeviceConfig.setOutputReport(smooth_x, smooth_y, 0);
}

// antiquated
void ArduinoMouse::click() {
    double delay = dis(gen);  // Random delay between 0.01 and 0.1 seconds
    
    DeviceConfig.setOutputReport(0, 0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
}

void ArduinoMouse::close() {
    DeviceConfig.stopHandle();
}
