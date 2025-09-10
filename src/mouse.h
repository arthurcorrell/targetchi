#pragma once

#include <Windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>

#pragma comment(lib, "hid.lib")

class ArduinoMouse {
private:

    void move(int x, int y);
    void click();

    std::vector<int> x_history;
    std::vector<int> y_history;
    size_t filter_length;

    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

public:
    ArduinoMouse(size_t filter_length = 3);
    ~ArduinoMouse();
    void set(int x, int y, bool b);
    void close();
};
