#pragma once


static constexpr unsigned short VENDOR_ID = 0x2341;   // Arduino VID
static constexpr unsigned short PRODUCT_ID = 0x8036;  // Leonardo PID
static constexpr unsigned char BUFFER_SIZE = 7;
static constexpr unsigned char REPORT_ID = 2;

// opaque pointer to ensure type safety 
typedef void* OPAQUEHANDLE;

// class has no private members, as internal state is limited to init.cpp translation unite
class DeviceConfig_ {

public:
    DeviceConfig_();
    void startHandle();
    void stopHandle();
    OPAQUEHANDLE getHandle();

    void setSaveByte(int b);
    void setOutputReport(int b1, int b2, int b3);

    bool activeHandle;
    bool deviceError;

};

// forward declaration- a pre-instantiated global object
extern DeviceConfig_ DeviceConfig;
/*
this is a design pattern where the class represents hardware- only one instance should exist, and it needs
to be instantiated correctly. Also, global access is needed

pointers are needed for:
polymorhpism- pointer types that point to derived classes. allows to access derived override functions with base class pointer
optional/nullable objects- when a feature might not exist
pointer to large objects- when the pointer length is smaller than the object size
*/