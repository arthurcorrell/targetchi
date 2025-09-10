// Choose which GUI implementation to use
// #include "config_gui.h"  // Windows native GUI
#include "config_raygui.h"   // RayGUI implementation
#include "aimbuddy.h"


#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <Windows.h>


// Settings controlled by GUI

// Default configuration
AimbuddyConfig config = {
    'P',                  // QUIT
    'Q',             // TOGGLE_KEY (ALT)
    
    'W',                  // TOGGLE_MOVE
    'E',          // ACTIVATE_MOVE (Right mouse button)
    99,                  // XFOV
    99,                  // YFOV
    2.0f / (5.0f * 0.54f), // MOVESPEED
    
    'W',                  // TOGGLE_TRIGGER
    VK_LBUTTON,           // ACTIVATE_TRIGGER (Left mouse button)
    true,                  // WASD_SAFETY
    3,                    // 0-indexed MouseType
    0                    // color masking color: 0=p, 1=r, 2=y
};


void checkBackendCV() {
    // Increase log level to see backend information
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_INFO);
    
    // Print OpenCV build information
    std::cout << "\n=== OpenCV Build Information ===\n";
    std::cout << cv::getBuildInformation() << std::endl;

    // Check CUDA availability
    std::cout << "\n=== CUDA Support ===\n";
    std::cout << "CUDA available: " << (cv::cuda::getCudaEnabledDeviceCount() > 0 ? "YES" : "NO") << std::endl;
    if (cv::cuda::getCudaEnabledDeviceCount() > 0) {
        cv::cuda::printCudaDeviceInfo(0); // Print info for first GPU
    }

    // Check parallel processing backends
    // std::cout << "\n=== Parallel Processing ===\n";
    // std::cout << "Parallel framework: " << cv::parallel::getNumThreads() << " threads available" << std::endl;

    // Check DNN backends
    std::cout << "\n=== DNN Module Backends ===\n";
    std::cout << "Available backends:" << std::endl;
    std::vector<std::pair<cv::dnn::Backend, cv::dnn::Target>> backends = {
        {cv::dnn::DNN_BACKEND_DEFAULT, cv::dnn::DNN_TARGET_CPU},
        {cv::dnn::DNN_BACKEND_HALIDE, cv::dnn::DNN_TARGET_CPU},
        {cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_CPU},
        {cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU},
        {cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_OPENCL},
        {cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_OPENCL_FP16},
        {cv::dnn::DNN_BACKEND_CUDA, cv::dnn::DNN_TARGET_CUDA},
        {cv::dnn::DNN_BACKEND_CUDA, cv::dnn::DNN_TARGET_CUDA_FP16},
        {cv::dnn::DNN_BACKEND_TIMVX, cv::dnn::DNN_TARGET_NPU},
        {cv::dnn::DNN_BACKEND_CANN, cv::dnn::DNN_TARGET_NPU}
    };

    for (const auto& backend : backends) {
        try {
            // Try to create a simple network to check if backend is available
            cv::dnn::Net net = cv::dnn::Net();
            net.setPreferableBackend(backend.first);
            net.setPreferableTarget(backend.second);
            std::cout << "- " << backend.first << " with target " << backend.second << ": AVAILABLE" << std::endl;
        }
        catch (...) {
            std::cout << "- " << backend.first << " with target " << backend.second << ": NOT AVAILABLE" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {

    //checkBackendCV();

    // Call blocking GUI to populate config
    config = ShowRayGUIConfigDialog(config);
    
    
    // Get screen dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    
    int CENTER_X = screen_width / 2;
    int CENTER_Y = screen_height / 2;
    
    AimBuddy aimbuddy(CENTER_X - config.XFOV / 2, CENTER_Y - config.YFOV / 2, 
                     config.XFOV, config.YFOV, config.MOVESPEED,
                     config.ACTIVATE_MOVE, config.ACTIVATE_TRIGGER, config.COLOR_MASK);
    
    // std::cout << "Targetchi started with configured settings" << std::endl;
    // std::cout << "Press " << (char)config.QUIT << " to quit" << std::endl;
    
    std::string status_move = "Disabled";
    std::string status_trigger = "Disabled";
    std::atomic<bool> running(true);
    
    // Capture a local copy of the quit key to avoid capturing global config
    int quitKey = config.QUIT;
    
    // Create a separate thread to check for keyboard interrupts
    std::thread interrupt_thread([&running, quitKey]() {
        while (running) {
            if (GetAsyncKeyState(quitKey) & 0x8000) {
                running = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    try {
        while (running) {
            if (GetAsyncKeyState(config.TOGGLE_KEY)) {
                if (GetAsyncKeyState(config.TOGGLE_MOVE) & 0x8000) {
                    aimbuddy.toggle_move();
                    status_move = aimbuddy.toggled_move ? "Enabled " : "Disabled";
                }
                if (GetAsyncKeyState(config.TOGGLE_TRIGGER) & 0x8000) {
                    aimbuddy.toggle_trigger();
                    status_trigger = aimbuddy.toggled_trigger ? "Enabled " : "Disabled";
                }
            }
            // std::cout << "Status move: " << status_move << std::endl;
            // std::cout << "Status trigger " << status_trigger << std::endl;
            // std::cout << std::flush;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const std::exception& e) {
        // std::cout << "Error:";
        // std::cout << std::endl;
    }
    
    // std::cout << "Exiting" << std::endl;
    // std::cout << std::endl;
    
    running = false;
    if (interrupt_thread.joinable()) {
        interrupt_thread.join();
    }
    
    return 0;
}
