#pragma once


static constexpr unsigned short VENDOR_ID = 0x2341;   // Arduino VID
static constexpr unsigned short PRODUCT_ID = 0x8036;  // Leonardo PID
static constexpr unsigned char BUFFER_SIZE = 7;
static constexpr unsigned char REPORT_ID = 2;

// opaque pointer to ensure type safety 
typedef void* OPAQUEHANDLE;

void startHandle();

void stopHandle();

void sendOutputReport(int b1, int b2, int b3);

OPAQUEHANDLE getHandle();


