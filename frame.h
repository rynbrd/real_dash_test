#ifndef __R51_FRAME__
#define __R51_FRAME__

#include <Arduino.h>

class Frame {
    public:
        Frame(uint32_t id, uint8_t len, uint8_t mask=0xFF);

        uint32_t id() const;
        uint8_t len() const;
        byte* data() const;

        // Set the frame data. Return true if the data differs from the
        // previous value.
        bool set(byte* data);
    private:
        uint32_t id_;
        uint8_t len_;
        byte data_[8];
        uint8_t mask_;
};

#endif  // __R51_FRAME__
