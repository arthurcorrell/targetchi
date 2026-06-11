#include "../include/init.h"
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <iostream>
#include <setupapi.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

static HANDLE dev = INVALID_HANDLE_VALUE;

// ---------------------------------------------------------------------------
// Writer infrastructure
//
// The handle is opened OVERLAPPED. Every write is issued and then waited to
// completion via GetOverlappedResult, so the report buffer and OVERLAPPED are
// always valid for the full duration of the USB transfer (the old code let a
// stack buffer / OVERLAPPED die before the interrupt transfer finished, which
// is what dropped reports under async).
//
// Movement is decoupled from the caller through a coalescing mailbox drained by
// g_writer. Relative deltas are additive, so when USB cannot keep up the
// pending deltas are summed rather than dropped; the firmware adds whatever we
// send to the host-shield mouse, so a coalesced report is exactly equivalent to
// the individual reports it replaces.
// ---------------------------------------------------------------------------

// Mirror of the private DeviceConfig_ constants for use by the file-static
// writer helpers (which are not class members and cannot see the private ones).
static constexpr BYTE  kReportId  = 2;  // == DeviceConfig_::REPORT_ID
static constexpr DWORD kReportLen = 7;  // == DeviceConfig_::BUFFER_SIZE

static std::mutex              g_state_mtx;   // guards the pending mailbox + cv
static std::condition_variable g_cv;
static long                    g_pending_dx = 0;
static long                    g_pending_dy = 0;
static int                     g_button = 0;
static bool                    g_has_work = false;

static std::mutex              g_write_mtx;   // serializes WriteFile on dev
static HANDLE                  g_write_event = nullptr;

static std::atomic<bool>       g_writer_run{false};
static std::thread             g_writer;

// Issue one report and block until the USB transfer actually completes.
// Safe to call with a stack buffer because we do not return until done.
static bool writeReportBlocking(const BYTE* data, DWORD len) {
    std::lock_guard<std::mutex> wl(g_write_mtx);
    if (dev == INVALID_HANDLE_VALUE || g_write_event == nullptr) {
        return false;
    }

    OVERLAPPED ov = {0};
    ResetEvent(g_write_event);
    ov.hEvent = g_write_event;

    DWORD written = 0;
    if (!WriteFile(dev, data, len, &written, &ov)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            return false;
        }
        // Wait for the interrupt OUT transfer to finish.
        if (!GetOverlappedResult(dev, &ov, &written, TRUE)) {
            return false;
        }
    }
    return written == len;
}

static void writerLoop() {
    while (g_writer_run.load(std::memory_order_acquire)) {
        long dx, dy;
        int  button;
        {
            std::unique_lock<std::mutex> lk(g_state_mtx);
            g_cv.wait(lk, [] {
                return g_has_work || !g_writer_run.load(std::memory_order_acquire);
            });
            if (!g_writer_run.load(std::memory_order_acquire)) {
                break;
            }

            // Take at most one int8 step per axis; the remainder stays pending
            // and is sent on the next iteration (a fast flick becomes a short
            // burst of max-speed reports paced naturally by the USB poll rate).
            long sx = g_pending_dx;
            if (sx >  127) sx =  127;
            if (sx < -127) sx = -127;
            long sy = g_pending_dy;
            if (sy >  127) sy =  127;
            if (sy < -127) sy = -127;

            g_pending_dx -= sx;
            g_pending_dy -= sy;
            dx = sx;
            dy = sy;
            button = g_button;

            // More to send only if deltas remain; a pure button update drains here.
            g_has_work = (g_pending_dx != 0 || g_pending_dy != 0);
        }

        BYTE report[kReportLen] = {
            kReportId,
            static_cast<BYTE>(static_cast<int8_t>(dx)),
            static_cast<BYTE>(static_cast<int8_t>(dy)),
            static_cast<BYTE>(button),
            0x00,
            0x00,
            0x00
        };
        writeReportBlocking(report, sizeof(report));
    }
}

static void startWriter() {
    if (g_writer_run.load()) {
        return;
    }
    if (g_write_event == nullptr) {
        g_write_event = CreateEvent(nullptr, TRUE, FALSE, nullptr); // manual-reset
    }
    {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        g_pending_dx = 0;
        g_pending_dy = 0;
        g_button = 0;
        g_has_work = false;
    }
    g_writer_run.store(true, std::memory_order_release);
    g_writer = std::thread(writerLoop);
}

static void stopWriter() {
    if (g_writer_run.load()) {
        g_writer_run.store(false, std::memory_order_release);
        g_cv.notify_all();
        // Unblock a write that is waiting on a transfer that will never finish
        // (e.g. device unplugged), so join() cannot hang.
        if (dev != INVALID_HANDLE_VALUE) {
            CancelIoEx(dev, nullptr);
        }
        if (g_writer.joinable()) {
            g_writer.join();
        }
    }
    if (g_write_event != nullptr) {
        CloseHandle(g_write_event);
        g_write_event = nullptr;
    }
}

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
        return;
    }

    // Device is live: spin up the writer thread that owns the movement mailbox.
    startWriter();

    return;
}


// closes handle. flips activeHandle flag to false
int DeviceConfig_::stopHandle() {
    // Stop the writer before the handle so no WriteFile is in flight on close.
    stopWriter();

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

Config-phase report (absolute values). Low frequency and order-sensitive, so
it is written synchronously rather than through the coalescing movement path.
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

    writeReportBlocking(reportBuffer, sizeof(reportBuffer));
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

    writeReportBlocking(reportBuffer, sizeof(reportBuffer));
}

// Runtime movement: deposit deltas into the mailbox and wake the writer.
// Never touches the handle, so the calling (aim) loop never blocks on USB.
void DeviceConfig_::queueMove(int dx, int dy, int button) {
    {
        std::lock_guard<std::mutex> lk(g_state_mtx);
        g_pending_dx += dx;
        g_pending_dy += dy;
        g_button = button;
        g_has_work = true;
    }
    g_cv.notify_one();
}

// type-safe API for referencing the handle. returns currently active handle
OPAQUEHANDLE getHandle() {
    return static_cast<OPAQUEHANDLE>(dev);
}

// instantiation that allocates memory
DeviceConfig_ DeviceConfig;
