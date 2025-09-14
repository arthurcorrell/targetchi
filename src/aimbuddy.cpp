#include "aimbuddy.h"
#include "config_gui.h"
#include <iostream>
#include <Windows.h>

AimBuddy::AimBuddy(int x, int y, int xfov, int yfov, float movespeed,
             int activate_move_key, int activate_trigger_key, int color_mask) : 
        movespeed(movespeed),
        activate_move_key(activate_move_key),
        activate_trigger_key(activate_trigger_key),
        toggled_move(false),
        toggled_trigger(false),
        gui_toggled(false),
        running(true),
        frame_count(0), 
        color_mask(color_mask), 
        x(x), y(y), xfov(xfov), yfov(yfov) {
    
        // Get desktop resolution
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Initialize screen buffer with continuous memory for optimal data transfer
    // Using continuous memory allocation helps with DMA and memory copies
    screen = cv::Mat(yfov, xfov, CV_8UC4);

    // Initialize DXGI resources
    if (!init_dxgi_duplication()) {
        std::cerr << "Failed to initialize DXGI screen duplication. Capture may not work." << std::endl;
    } else {
        std::cout << "DXGI screen duplication initialized successfully." << std::endl;
    }

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
    hsv.create(screen.size(), CV_8UC3);
    mask.create(screen.size(), CV_8UC1);
    dilated.create(screen.size(), CV_8UC1);
          
    // Start listening thread
    listen_thread = std::thread(&AimBuddy::listen, this);
}

void AimBuddy::update_fps() {
    frame_count++;
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
    
    if (elapsed_time >= 0.1) {
        double fps = (frame_count / static_cast<double>(elapsed_time)) * 10.0f;
        std::cout << "DXGI Capture - FPS: " << static_cast<int>(fps) << "\r" << std::flush;
        frame_count = 0;
        start_time = current_time;
    }
}

bool AimBuddy::capture_screen() {
    // Skip if DXGI duplication is not initialized
    if (!dxgi_output_duplication) {
        return false;
    }

    // Variables to hold frame data
    IDXGIResource* desktop_resource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    HRESULT hr = S_OK;

    // Try to acquire the next frame with minimal timeout (8ms)
    hr = dxgi_output_duplication->AcquireNextFrame(8, &frame_info, &desktop_resource);
    
    // If timeout or no new frame is available
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // No new frame available, we can continue
        return false;
    } else if (FAILED(hr)) {
        std::cerr << "Failed to acquire frame. Error code: 0x" << std::hex << hr << std::endl;
        
        // If the duplication interface is invalid (e.g., screen resolution change), reinitialize it
        if (hr == DXGI_ERROR_ACCESS_LOST) {
            std::cout << "Access lost to duplication interface. Reinitializing..." << std::endl;
            release_dxgi_resources();
            if (!init_dxgi_duplication()) {
                std::cerr << "Failed to reinitialize DXGI duplication." << std::endl;
            }
        }
        return false;
    }

    // Get texture interface from resource
    hr = desktop_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&desktop_texture));
    desktop_resource->Release();
    
    if (FAILED(hr)) {
        std::cerr << "Failed to get texture from resource. Error code: 0x" << std::hex << hr << std::endl;
        dxgi_output_duplication->ReleaseFrame();
        return false;
    }

    // Get texture description
    D3D11_TEXTURE2D_DESC texture_desc;
    desktop_texture->GetDesc(&texture_desc);

    // Calculate the region to capture based on x, y, xfov, yfov
    // Make sure region is within screen bounds
    int capture_x = std::max(0, std::min(x, static_cast<int>(texture_desc.Width) - 1));
    int capture_y = std::max(0, std::min(y, static_cast<int>(texture_desc.Height) - 1));
    int capture_width = std::min(xfov, static_cast<int>(texture_desc.Width) - capture_x);
    int capture_height = std::min(yfov, static_cast<int>(texture_desc.Height) - capture_y);

    // Define the box to copy from the desktop texture
    D3D11_BOX src_box;
    src_box.left = capture_x;
    src_box.top = capture_y;
    src_box.right = capture_x + capture_width;
    src_box.bottom = capture_y + capture_height;
    src_box.front = 0;
    src_box.back = 1;

    // Copy the region from the desktop texture to the staging texture
    d3d_context->CopySubresourceRegion(staging_texture, 0, 0, 0, 0, desktop_texture, 0, &src_box);
    desktop_texture->Release();
    desktop_texture = nullptr;

    // Map the staging texture to CPU accessible memory
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    hr = d3d_context->Map(staging_texture, 0, D3D11_MAP_READ, 0, &mapped_resource);
    
    if (FAILED(hr)) {
        std::cerr << "Failed to map staging texture. Error code: 0x" << std::hex << hr << std::endl;
        dxgi_output_duplication->ReleaseFrame();
        return false;
    }

    // Copy data from the mapped resource to our OpenCV Mat
    // We have to account for the source row pitch which might be different from the destination
    for (int row = 0; row < capture_height; ++row) {
        const BYTE* source_row = static_cast<BYTE*>(mapped_resource.pData) + (row * mapped_resource.RowPitch);
        BYTE* dest_row = screen.ptr(row);
        
        // Copy one row at a time (BGRA data)
        memcpy(dest_row, source_row, capture_width * 4); // 4 bytes per pixel (BGRA)
    }
    
    // Unmap the staging texture
    d3d_context->Unmap(staging_texture, 0);
    
    // Release the frame
    hr = dxgi_output_duplication->ReleaseFrame();
    if (FAILED(hr)) {
        std::cerr << "Failed to release frame. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    return true;
}

bool AimBuddy::init_dxgi_duplication() {
    HRESULT hr = S_OK;
    
    // Create D3D11 device
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;
    
    hr = D3D11CreateDevice(
        nullptr,                    // Default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
        nullptr,                    // No software device
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED, // Optimize for single-threaded access
        featureLevels,              // Feature levels
        ARRAYSIZE(featureLevels),   // # of feature levels
        D3D11_SDK_VERSION,          // SDK version
        &d3d_device,                // Output device
        &featureLevel,              // Output feature level
        &d3d_context                // Output context
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create D3D11 device. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Get DXGI device
    IDXGIDevice* dxgi_device = nullptr;
    hr = d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));
    if (FAILED(hr)) {
        std::cerr << "Failed to get DXGI device. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Get DXGI adapter
    IDXGIAdapter* dxgi_adapter = nullptr;
    hr = dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
    dxgi_device->Release();
    if (FAILED(hr)) {
        std::cerr << "Failed to get DXGI adapter. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Get primary output (monitor)
    IDXGIOutput* dxgi_output = nullptr;
    hr = dxgi_adapter->EnumOutputs(0, &dxgi_output);  // Primary display = 0
    dxgi_adapter->Release();
    if (FAILED(hr)) {
        std::cerr << "Failed to get DXGI output. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Get DXGI Output1 interface
    IDXGIOutput1* dxgi_output1 = nullptr;
    hr = dxgi_output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&dxgi_output1));
    dxgi_output->Release();
    if (FAILED(hr)) {
        std::cerr << "Failed to get DXGI Output1. Error code: 0x" << std::hex << hr << std::endl;
        return false;
    }
    
    // Create desktop duplication interface
    hr = dxgi_output1->DuplicateOutput(d3d_device, &dxgi_output_duplication);
    dxgi_output1->Release();
    if (FAILED(hr)) {
        std::cerr << "Failed to create output duplication. Error code: 0x" << std::hex << hr << std::endl;
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
            std::cerr << "Maximum number of applications using DXGI duplication API reached." << std::endl;
        }
        return false;
    }
    
    // Create staging texture for CPU readback
    D3D11_TEXTURE2D_DESC texture_desc = {};
    texture_desc.Width = xfov;
    texture_desc.Height = yfov;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // BGRA format
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    hr = d3d_device->CreateTexture2D(&texture_desc, nullptr, &staging_texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create staging texture. Error code: 0x" << std::hex << hr << std::endl;
        dxgi_output_duplication->Release();
        dxgi_output_duplication = nullptr;
        return false;
    }
    
    return true;
}

void AimBuddy::release_dxgi_resources() {
    // Release all DXGI resources in reverse order of creation
    if (staging_texture) {
        staging_texture->Release();
        staging_texture = nullptr;
    }
    
    if (desktop_texture) {
        desktop_texture->Release();
        desktop_texture = nullptr;
    }
    
    if (dxgi_output_duplication) {
        dxgi_output_duplication->Release();
        dxgi_output_duplication = nullptr;
    }
    
    if (d3d_context) {
        d3d_context->Release();
        d3d_context = nullptr;
    }
    
    if (d3d_device) {
        d3d_device->Release();
        d3d_device = nullptr;
    }
}


AimBuddy::~AimBuddy() {
    {
        // Lock to ensure no other thread is accessing screen
        std::lock_guard<std::mutex> guard(lock);
        // Release DXGI resources
        release_dxgi_resources();
    }
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
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void AimBuddy::toggle_trigger() {
    toggled_trigger = !toggled_trigger;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


void AimBuddy::listen() {
    while (running) {
        bool m {(GetAsyncKeyState(activate_move_key) & 0x8000) && toggled_move};
        bool c {(GetAsyncKeyState(activate_trigger_key) & 0x8000) && toggled_trigger};
        if (capture_screen()) {
            if (m || c) {
                process(m, c);
                update_fps();
            }
        }

        //std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));  // limit to 140 fps
    }
}

void AimBuddy::process(bool m, bool c) {
    cv::cvtColor(screen, hsv, cv::COLOR_BGR2HSV);
    
    cv::inRange(hsv, lower_color, upper_color, mask);
    
    // Dilate to improve contour detection
    cv::dilate(mask, dilated, cv::Mat(), cv::Point(-1, -1), 5);

    cv::imshow("Debug Capture", mask);
    cv::waitKey(1);
    
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
        int x_diff = cX - xfov / 2;
        int y_diff = cY - yfov / 2;
        arduinomouse.set(static_cast<int>(x_diff * movespeed), static_cast<int>(y_diff * movespeed), c);
    } else if (c && 
             std::abs(center.x - xfov / 2) <= 4 && 
             std::abs(center.y - yfov / 2) <= 10) {
        arduinomouse.set(0, 0, c);
    }
    arduinomouse.set(1, 1, 0);
}
