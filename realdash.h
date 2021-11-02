#ifndef __R51_REALDASH_H__
#define __R51_REALDASH_H__

#include <Arduino.h>
#include "CRC32.h"
#include "receiver.h"

/* RealDash Control Frames
 *
 * Frame 0x5400: Climate Controls
 *   Byte 0: Unit Status
 *     Bit 0: active; send 0 to deactivate, 1 to enable auto mode
 *     Bit 1: auto
 *     Bit 2: ac
 *     Bit 3: dual
 *     Bit 4: mode; command only; flip to trigger
 *     Bit 5: face; status only
 *     Bit 6: feet; status only
 *     Bit 7: recirculate
 *   Byte 1: defrost
 *     Bit 0: defrost front
 *     Bit 1: defrost rear
 *     Bit 2: defrost mirrors
 *     Bit 3-7: unused
 *   Byte 2: fan speed
 *     Bits 0-3: fan speed; status only
 *     Bit 4: fan+; command only; flip to trigger
 *     Bit 5: fan-; command only; flip to trigger
 *     Buts 6-7: unused
 *   Byte 3: driver temp
 *     Bits 0-8: driver temp
 *   Byte 4: passenger temp
 *     Bits 0-8: passenger temp
 */

// Reads and writes frames to RealDash over serial. Supports RealDash 0x44 and
// 0x66 type frames. All written frames are 0x66 for error checking (0x44
// frames do not contain a checksum).
class RealDashReceiver : public Receiver {
    public:
        // Construct an uninitialized RealDash instance.
        RealDashReceiver();

        // Start the RealDash instance. Data is transmitted over the given
        // serial stream. This is typically Serial or SerialUSB.
        void begin(Stream* stream);

        // Read a frame from RealDash. Returns true if a frame was read or
        // false if not. Should be called on every loop or the connected serial
        // device may block.
        bool read(uint32_t* id, uint8_t* len, byte* data) override;

        // Write frame to RealDash. Return false on success or false on
        // failure.
        bool write(uint32_t id, uint8_t len, byte* data) override;

    private:
        Stream* stream_;

        // Read attributes.
        bool frame_type_66_;        // Type of frame. False if 0x44, true if 0x66.
        uint8_t frame44_checksum_;  // Frame 0x44 checksum.  Calculated as bytes are read.
        CRC32 frame66_checksum_;    // Frame 0x66 checksum. Calculated as bytes are read.
        byte checksum_buffer_[4];   // Buffer to read in the checksum.
        uint8_t frame_size_;        // Expected size of the frame data if type is 0x66.
        uint8_t read_size_;         // Tracks how many bytes have been read.

        // Write attributes.
        CRC32 write_checksum_;

        void updateChecksum(byte b);
        bool readHeader();
        bool readId(uint32_t* id);
        bool readData(uint8_t* len, byte* data);
        bool validateChecksum();
        void reset();
        void writeByte(const byte b);
        void writeBytes(const byte* b, uint8_t len);
};

#endif
