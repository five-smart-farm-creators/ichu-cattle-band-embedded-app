#include "ReadingBuffer.h"

bool ReadingBuffer::push(const TelemetryReading& reading) {
    const bool dropped = count_ == kCapacity;
    if (dropped) {
        head_ = (head_ + 1) % kCapacity;
        --count_;
    }
    readings_[(head_ + count_) % kCapacity] = reading;
    ++count_;
    return dropped;
}

const TelemetryReading& ReadingBuffer::front() const {
    return readings_[head_];
}

void ReadingBuffer::pop() {
    head_ = (head_ + 1) % kCapacity;
    --count_;
}

size_t ReadingBuffer::size() const {
    return count_;
}

bool ReadingBuffer::empty() const {
    return count_ == 0;
}
