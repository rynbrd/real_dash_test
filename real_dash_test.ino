#include <Arduino.h>
#include "can.h"
#include "debug.h"
#include "mcp_can_dfs.h"
#include "realdash.h"
#include "serial.h"

#define CAN_CS_PIN 17
#define CAN_BAUDRATE CAN_250KBPS
#define CAN_CLOCK MCP_16MHZ

RealDashReceiver rd;
SerialReceiver sr;
CanReceiver cr;

Receiver* receivers[] = {&rd, &sr, &cr};

uint32_t id;
uint8_t len;
byte data[64];

void write() {
    if (id < 0x1000) {
        cr.write(id, len, data);
    } else {
        rd.write(id, len, data);
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);

    INFO_MSG("waiting for usb serial");
    while (!Serial) {
        delay(100);
    }
    INFO_MSG("usb serial ready");
    rd.begin(&Serial1);
    sr.begin(&Serial);
    while(!cr.begin(CAN_CS_PIN, CAN_BAUDRATE, CAN_CLOCK)) {
        delay(1000);
    }
    INFO_MSG("initialized");
}

void loop() {
    for (uint8_t i = 0; i < sizeof(receivers)/sizeof(Receiver*); i++) {
        if (receivers[i]->read(&id, &len, data)) {
            INFO_MSG_FRAME("realdash receive ", id, len, data);
            write();
        }
    }
}
