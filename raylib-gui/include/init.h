#pragma once

#ifdef RAYLIB_GUI_EXPORTS
    #define RAYLIB_GUI_API __declspec(dllexport)
#else
    #define RAYLIB_GUI_API __declspec(dllimport)
#endif


// opaque pointer to ensure type safety 
typedef void* OPAQUEHANDLE;

// class has no private members, as internal state is limited to init.cpp translation unite
class RAYLIB_GUI_API DeviceConfig_ {

public:
    DeviceConfig_();
    void startHandle();
    int stopHandle();
    OPAQUEHANDLE getHandle();

    void setSaveByte(int b);
    void setOutputReport(int b1, int b2, int b3);

    // Runtime movement path: non-blocking, coalescing. Deltas are accumulated
    // and drained by a dedicated writer thread so the caller (aim loop) never
    // stalls on USB and congested writes merge instead of being dropped.
    void queueMove(int dx, int dy, int button);

    bool activeHandle;
    bool deviceError;
private:
    static constexpr unsigned short VENDOR_ID = 0x2341;   // Arduino VID
    static constexpr unsigned short PRODUCT_ID = 0x8036;  // Leonardo PID

    // static constexpr unsigned short VENDOR_ID = 0x258B;   // glorious VID
    // static constexpr unsigned short PRODUCT_ID = 0x1234;  // random PID lol
    static constexpr unsigned char BUFFER_SIZE = 7;
    static constexpr unsigned char REPORT_ID = 2;

};

// forward declaration- a pre-instantiated global object
extern RAYLIB_GUI_API DeviceConfig_ DeviceConfig;
/*
this is a design pattern where the class represents hardware- only one instance should exist, and it needs
to be instantiated correctly. Also, global access is needed

pointers are needed for:
polymorhpism- pointer types that point to derived classes. allows to access derived override functions with base class pointer
optional/nullable objects- when a feature might not exist
pointer to large objects- when the pointer length is smaller than the object size
*/