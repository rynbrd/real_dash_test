#include "frame.h"

Frame::Frame(uint32_t id, uint8_t len, uint8_t mask) {
    id_ = id;
    len_ = len;
    memset(data_, 0, 8);
}

uint32_t Frame::id() const { return id_; }
uint8_t Frame::len() const { return len_; }
byte* Frame::data() const { return data_; }

bool Frame::set(byte* data) {
    bool changed = false;
    for (int i = 0; i < len_; i++) {
        bool ignore = ((mask_ >> i) & 0x01) == 0;
        if (!ignore && data[i] != data_[i]) {
            changed = true;
        }
        data_[i] = data[i];
    }
    return changed;
}
