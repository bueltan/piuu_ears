#pragma once
#include <stdint.h>
#include <stddef.h>

// Incremental control framing shared by the two links. Payload bytes may contain
// magic; never search for another header until the declared payload is consumed.
// Oversize/unknown packets are consumed without allocating their payload.
class ControlParser
{
public:
    using Handler = void (*)(uint8_t version, uint8_t code,
                             const uint8_t *payload, size_t size);
    ControlParser(const uint8_t *magic, Handler handler)
        : magic_(magic), handler_(handler) {}
    void expire(uint32_t now, uint32_t timeout)
    {
        if (inProgress() && uint32_t(now - lastByte_) >= timeout) reset();
    }
    void feed(uint8_t byte, uint32_t now)
    {
        lastByte_ = now;
        if (state_ == Search) {
            matched_ = byte == magic_[matched_] ? matched_ + 1 : (byte == magic_[0] ? 1 : 0);
            if (matched_ == 4) { matched_ = 0; used_ = 0; state_ = Header; }
            return;
        }
        if (state_ == Header) {
            header_[used_++] = byte;
            if (used_ != sizeof(header_)) return;
            length_ = uint16_t(header_[2]) | (uint16_t(header_[3]) << 8);
            used_ = 0;
            if (!length_) { deliver(); return; }
            state_ = length_ <= sizeof(payload_) ? Payload : Discard;
            return;
        }
        if (state_ == Payload) payload_[used_] = byte;
        ++used_;
        if (used_ == length_) {
            if (state_ == Payload) deliver(); else reset();
        }
    }
private:
    enum State { Search, Header, Payload, Discard } state_ = Search;
    const uint8_t *magic_;
    Handler handler_;
    uint8_t header_[4] = {};
    uint8_t payload_[32] = {};
    size_t matched_ = 0, used_ = 0;
    uint16_t length_ = 0;
    uint32_t lastByte_ = 0;
    bool inProgress() const { return state_ != Search || matched_ != 0; }
    void reset() { state_ = Search; matched_ = used_ = 0; length_ = 0; }
    void deliver()
    {
        handler_(header_[0], header_[1], length_ ? payload_ : nullptr, length_);
        reset();
    }
};
