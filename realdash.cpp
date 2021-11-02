#include "realdash.h"

#include <Arduino.h>
#include "debug.h"


RealDashReceiver::RealDashReceiver() {
    stream_ = nullptr;
    reset();
}

void RealDashReceiver::reset() {
    memset(checksum_buffer_, 0, 4);
    frame_type_66_ = false;
    frame44_checksum_ = 0;
    frame66_checksum_.reset();
    frame_size_ = 8;
    read_size_ = 0;
}

void RealDashReceiver::begin(Stream* stream) {
    stream_ = stream;
}

void frameChecksumError(uint32_t checksum) {
    ERROR_MSG_VAL_FMT("realdash: frame checksum error, wanted ", checksum, HEX);
}

void RealDashReceiver::updateChecksum(byte b) {
    if (frame_type_66_) {
        frame66_checksum_.update(b);
    } else {
        frame44_checksum_ += b;
    }
}

bool RealDashReceiver::read(uint32_t* id, uint8_t* len, byte* data) {
    if (stream_ == nullptr) {
        ERROR_MSG("realdash: not initialized");
        return false;
    }
    if (readHeader() && readId(id) && readData(len, data) && validateChecksum()) {
        reset();
        return true;
    }
    return false;
}

bool RealDashReceiver::readHeader() {
    byte b;
    while (stream_->available() && read_size_ < 4) {
        b = stream_->read();
        read_size_++;
        switch (read_size_) {
            case 1:
                if (b != 0x44 && b != 0x66) {
                    ERROR_MSG_VAL_FMT("realdash: unrecognized frame type ", b, HEX);
                    reset();
                    return false;
                }
                frame_type_66_ = (b == 0x66);
                break;
            case 2:
                if (b != 0x33) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 2 ", b, HEX);
                    reset();
                    return false;
                }
                break;
            case 3:
                if (b != 0x22) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 3 ", b, HEX);
                    reset();
                    return false;
                }
                break;
            case 4:
                if ((!frame_type_66_ && b != 0x11) || (frame_type_66_ && (b < 0x11 || b > 0x1F))) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 4 ", b, HEX);
                    reset();
                    return false;
                }
                frame_size_ = (b - 15) * 4;
                break;
            default:
                break;
        }
        updateChecksum(b);
    }
    return read_size_ >= 4;
}

bool RealDashReceiver::readId(uint32_t* id) {
    uint32_t b;
    while (stream_->available() && read_size_ < 8) {
        b = stream_->read();
        read_size_++;
        updateChecksum(b);
        switch (read_size_-4) {
            case 1:
                *id = b;
                break;
            case 2:
                *id |= (b << 8);
                break;
            case 3:
                *id |= (b << 16);
                break;
            case 4:
                *id |= (b << 24);
                break;
        }
    }
    return read_size_ >= 8;
}

bool RealDashReceiver::readData(uint8_t* len, byte* data) {
    while (stream_->available() && read_size_ - 8 < frame_size_) {
        data[read_size_-8] = stream_->read();
        updateChecksum(data[read_size_-8]);
        read_size_++;
    }
    *len = frame_size_;
    return read_size_ - 8 >= frame_size_;
}

bool RealDashReceiver::validateChecksum() {
    if (frame_type_66_) {
        while (stream_->available() && read_size_ - 8 - frame_size_ < 4) {
            checksum_buffer_[read_size_ - 8 - frame_size_] = stream_->read();
            read_size_++;
        }
        if (read_size_ - 8 - frame_size_ < 4) {
            return false;
        }
        if (frame66_checksum_.finalize() != *((uint32_t*)checksum_buffer_)) {
            frameChecksumError(frame66_checksum_.finalize());
            reset();
            return false;
        }
    } else {
        if (read_size_ - 8 - frame_size_ < 1) {
            if (!stream_->available()) {
                return false;
            }
            checksum_buffer_[0] = stream_->read();
            read_size_++;
        }
        if (frame44_checksum_ != checksum_buffer_[0]) {
            frameChecksumError(frame44_checksum_);
            reset();
            return false;
        }
    }
    return true;
}

void RealDashReceiver::writeByte(const byte b) {
    stream_->write(b);
    write_checksum_.update(b);
}

void RealDashReceiver::writeBytes(const byte* b, uint8_t len) {
    for (int i = 0; i < len; i++) {
        writeByte(b[i]);
    }
}

bool RealDashReceiver::write(uint32_t id, uint8_t len, byte* data) {
    if (stream_ == nullptr) {
        ERROR_MSG("realdash: not initialized");
        return false;
    }
    if (data == nullptr) {
        len = 0;
    }
    if (len > 64 || len % 4 != 0) {
        ERROR_MSG_VAL("realdash: frame write error, invalid length ", len);
        return false;
    }

    write_checksum_.reset();

    byte size = len / 4 + 15;
    writeByte(0x66);
    writeByte(0x33);
    writeByte(0x22);
    writeByte(size);
    writeBytes((const byte*)&id, 4);
    if (data != nullptr) {
        writeBytes(data, len);
    }
    for (int i = 0; i < 8-len; i++) {
        writeByte(0);
    }
    uint32_t checksum = write_checksum_.finalize();
    stream_->write((const byte*)&checksum, 4);
    stream_->flush();
    return true;
}
