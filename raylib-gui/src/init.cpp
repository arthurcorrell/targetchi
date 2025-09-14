#include "../include/init.h"
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <iostream>
#include <setupapi.h>
#include <chrono>
#include <thread>

static HANDLE dev = INVALID_HANDLE_VALUE;

DeviceConfig_::DeviceConfig_() : activeHandle(false), deviceError(false) {

    // track whether handle is active

    // track if error is thrown by handle connection

}
// finds device handle. flips activeHandle flag to true
void DeviceConfig_::startHandle() {    
    
    // Get device info set for HID devices
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        // std::cerr << "Driver Error: Failed to get device list" << std::endl;
        deviceError = true;
        activeHandle = false;
        return;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Enumerate all HID devices
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, i, &deviceInterfaceData); i++) {
        DWORD requiredSize = 0;
        
        // Get the required buffer size
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);
        
        // Allocate memory for the interface detail data
        PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = 
            (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        
        if (!detailData) {
            continue;
        }
        
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        
        // Get the interface detail data
        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, 
                                         detailData, requiredSize, nullptr, nullptr)) {
            // Open the device
            HANDLE tempHandle = CreateFile(
                detailData->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED, // changed from 0
                nullptr);
                
            if (tempHandle != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES deviceAttributes;
                deviceAttributes.Size = sizeof(HIDD_ATTRIBUTES);

                //std::cout << "current device handle: " << tempHandle << std::endl;
                
                if (HidD_GetAttributes(tempHandle, &deviceAttributes) &&
                    deviceAttributes.VendorID == VENDOR_ID &&
                    deviceAttributes.ProductID == PRODUCT_ID) {

                    // Found our device
                    // Get preparsed data to identify the correct HID interface
                    PHIDP_PREPARSED_DATA preparsedData = nullptr;
                    if (HidD_GetPreparsedData(tempHandle, &preparsedData)) {
                        HIDP_CAPS capabilities;
                        if (HidP_GetCaps(preparsedData, &capabilities) == HIDP_STATUS_SUCCESS) {
                            //std::cout << "Device capabilities:" << std::endl;
                            //std::cout << "  Usage: " << capabilities.Usage << std::endl;
                            //std::cout << "  UsagePage: " << capabilities.UsagePage << std::endl;
                            //std::cout << "  OutputReportByteLength: " << capabilities.OutputReportByteLength << std::endl;
                        }
                        HidD_FreePreparsedData(preparsedData);
                        /*
                        Device capabilities:
                            Usage: 1
                            UsagePage: 99
                            OutputReportByteLength: 7
                            Found device matching VID:PID 2341:8036
                        */
                    }
                    
                    // Found our device
                    dev = tempHandle;

                    activeHandle = true;
                    deviceError = false;
                    // ended in 134

                    /*
                    std::cout << "Found correct device handle: " << tempHandle << std::endl;
                    
                    std::cout << "Found device matching VID:PID " << std::hex << VENDOR_ID << ":" << PRODUCT_ID << std::dec << std::endl;
                    std::cout << "VID: " << std::hex << deviceAttributes.VendorID << 
                    ", PID: " << deviceAttributes.ProductID << std::dec << std::endl;

                    // Print the device path for debugging
                    std::wcout << L"Device path: " << detailData->DevicePath << std::endl;
                    */

                    free(detailData);
                    break;
                }
                
                CloseHandle(tempHandle);
            } else {
                DWORD error = GetLastError();
                //std::cerr << "Failed to open device. Error code: " << error << std::endl;
            }
        }
        
        free(detailData);
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
    if (dev == INVALID_HANDLE_VALUE) {
        // std::cerr << "ERROR: Cannot find device. Make sure that device is plugged in and has been flashed" << std::endl;
        deviceError = true;
        activeHandle = false;
        // std::this_thread::sleep_for(std::chrono::seconds(5));
        //exit(EXIT_FAILURE);
    }

    return;
}


// closes handle. flips activeHandle flag to false
int DeviceConfig_::stopHandle() {
    if (dev != INVALID_HANDLE_VALUE) {
        int ret = CloseHandle(dev);
        dev = INVALID_HANDLE_VALUE;
        activeHandle = false;
        return ret;
    }
    return 0;
}

/*
set output reports bytes 1, 2, 3
Byte 1: 0-indexed mouseType
Byte 2: Mouse polling rate 8, 4, 2, 1
Byte 3: movespeed [0-100] (as float)

Set to -1 if no change
*/
void DeviceConfig_::setOutputReport(int b1, int b2, int b3) {
    BYTE reportBuffer[BUFFER_SIZE] = {
        REPORT_ID,
        static_cast<BYTE>(b1),
        static_cast<BYTE>(b2),
        static_cast<BYTE>(b3),
        0x00,
        0x00,
        0x00
    };

    // Send HID output report
    // if (!HidD_SetOutputReport(dev, reportBuffer, sizeof(reportBuffer))) {
    //     std::cerr << "Failed to send HID report" << std::endl;
    // }

    DWORD bytesWritten = 0;
    OVERLAPPED overlapped = {0};
    
    // Use WriteFile for sending to the interrupt endpoint. returns 0 when async
    // WriteFile(dev, reportBuffer, sizeof(reportBuffer), &bytesWritten, &overlapped);

    //overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    // Use WriteFile for sending to the interrupt endpoint. returns 0 when async
    // OVERLAPPED struct makes operation async
    WriteFile(dev, reportBuffer, sizeof(reportBuffer), &bytesWritten, &overlapped);
    // WaitForSingleObject(overlapped.hEvent, INFINITE); // Wait for completion
    // CloseHandle(overlapped.hEvent);
}

// set byte 6 to 169 to exit configuration loop
void DeviceConfig_::setSaveByte(int b) {
    BYTE reportBuffer[BUFFER_SIZE] = {
        REPORT_ID,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        static_cast<BYTE>(b)
    };

    // Send HID output report
    // if (!HidD_SetOutputReport(dev, reportBuffer, sizeof(reportBuffer))) {
    //     std::cerr << "Failed to send HID report" << std::endl;
    // }

    DWORD bytesWritten = 0;
    OVERLAPPED overlapped = {0};
    
    // Use WriteFile for sending to the interrupt endpoint. returns 0 when async
    WriteFile(dev, reportBuffer, sizeof(reportBuffer), &bytesWritten, &overlapped);
}

// type-safe API for referencing the handle. returns currently active handle
OPAQUEHANDLE getHandle() {
    return static_cast<OPAQUEHANDLE>(dev);
}

// instantiation that allocates memory
DeviceConfig_ DeviceConfig;