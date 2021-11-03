#include <Arduino.h>
#include "can.h"
#include "debug.h"
#include "mcp_can_dfs.h"
#include "realdash.h"
#include "serial.h"
#include "frame.h"

#define CAN_CS_PIN 17
#define CAN_BAUDRATE CAN_500KBPS
#define CAN_CLOCK MCP_16MHZ

//RealDashReceiver rd;
SerialReceiver sr;
CanReceiver cr;

Receiver* receivers[] = {
    //&rd,
    &sr,
    &cr,
};

uint32_t id;
uint8_t len;
byte data[64];

Frame frame35D(0x35D, 8);
Frame frame540(0x540, 8);
Frame frame541(0x541, 8);
Frame frame54A(0x54A, 8, 0xFE);
Frame frame54B(0x54B, 8, 0x7F);
Frame frame625(0x625, 6);

Frame* frames[] = {
    &frame35D,
    &frame540,
    &frame541,
    &frame54A,
    &frame54B,
    &frame625,
};

uint32_t last_send;

void printFrameIn(uint32_t id, uint8_t len, byte* data) {
    DEBUG_SERIAL.print("<< ");
    printDebugFrame(id, len, data);
    DEBUG_SERIAL.println("");
}

void printFrameOut(uint32_t id, uint8_t len, byte* data) {
    DEBUG_SERIAL.print(">> ");
    printDebugFrame(id, len, data);
    DEBUG_SERIAL.println("");
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);

    INFO_MSG("waiting for usb serial");
    while (!Serial) {
        delay(100);
    }
    INFO_MSG("usb serial ready");

    //rd.begin(&Serial1);
    sr.begin(&Serial);
    while(!cr.begin(CAN_CS_PIN, CAN_BAUDRATE, CAN_CLOCK)) {
        delay(1000);
    }

    last_send = millis();
    frame540.data()[0] = 0x80;
    frame541.data()[0] = 0x80;
    INFO_MSG("initialized");
}

void loop() {
    if (cr.read(&id, &len, data)) {
        for (int i = 0; i < sizeof(frames)/sizeof(frames[0]); i++) {
            if (frames[i]->id() == id) {
                if (frames[i]->set(data)) {
                    printFrameIn(id, len, data);
                }
            }
        }
    }

    bool changed = false;
    if (sr.read(&id, &len, data)) {
        switch (id) {
            case 0x540:
                changed = frame540.set(data);
                break;
            case 0x541:
                changed = frame541.set(data);
                break;
            default:
                printFrameOut(id, len, data);
                if (!cr.write(id, len, data)) {
                    DEBUG_SERIAL.println("write failed");
                }
                break;
        }
    }
    if (changed || millis() - last_send >= 100) {
        if (changed) {
            printFrameOut(frame541.id(), frame541.len(), frame541.data());
        }
        if (!cr.write(frame541.id(), frame541.len(), frame541.data())) {
            DEBUG_SERIAL.println("write failed");
        }
        if (changed) {
            printFrameOut(frame540.id(), frame540.len(), frame540.data());
        }
        if (!cr.write(frame540.id(), frame540.len(), frame540.data())) {
            DEBUG_SERIAL.println("write failed");
        }
        last_send = millis();
    }
}
