#include "ReadingBuffer.h"

#include <Arduino.h>
#include <stdio.h>

namespace {

constexpr char kQueueNamespace[] = "ichu-queue";
// Head and count share one value so a power cut can never store one without the other.
constexpr char kStateKey[] = "state";
constexpr char kLayoutKey[] = "layout";
// Slot keys are "r00" to "r59" plus the terminator; the static_assert below keeps two digits enough.
constexpr size_t kSlotKeyCapacity = 4;

void slotKey(size_t slot, char* key) {
    snprintf(key, kSlotKeyCapacity, "r%02u", static_cast<unsigned>(slot));
}

}  // namespace

static_assert(CollarConfig::kBufferCapacity <= 100, "slot keys have two digits");

void ReadingBuffer::begin() {
    preferences_.begin(kQueueNamespace, false);
    if (restore()) {
        return;
    }
    // Entries stored with another layout would be read as garbage, so the queue starts empty.
    preferences_.clear();
    preferences_.putUInt(kLayoutKey, TelemetryReading::kLayoutVersion);
    head_ = 0;
    count_ = 0;
}

bool ReadingBuffer::push(const TelemetryReading& reading) {
    const bool dropped = count_ == kCapacity;
    // A full queue reuses the oldest slot, so the slot is written before the counters move.
    const size_t slot = dropped ? head_ : (head_ + count_) % kCapacity;
    char key[kSlotKeyCapacity];
    slotKey(slot, key);
    if (preferences_.putBytes(key, &reading, sizeof(reading)) != sizeof(reading)) {
        Serial.println("[buffer] NVS write failed, reading not queued");
        return false;
    }
    readings_[slot] = reading;
    if (dropped) {
        head_ = (head_ + 1) % kCapacity;
    } else {
        ++count_;
    }
    saveCounters();
    return dropped;
}

const TelemetryReading& ReadingBuffer::front() const {
    return readings_[head_];
}

void ReadingBuffer::pop() {
    head_ = (head_ + 1) % kCapacity;
    --count_;
    saveCounters();
}

size_t ReadingBuffer::size() const {
    return count_;
}

bool ReadingBuffer::empty() const {
    return count_ == 0;
}

bool ReadingBuffer::restore() {
    if (preferences_.getUInt(kLayoutKey, 0) != TelemetryReading::kLayoutVersion) {
        return false;
    }
    const uint32_t state = preferences_.getUInt(kStateKey, 0);
    const uint32_t head = state >> 16;
    const uint32_t count = state & 0xFFFF;
    if (head >= kCapacity || count > kCapacity) {
        return false;
    }
    head_ = head;
    count_ = count;
    for (size_t i = 0; i < count_; ++i) {
        const size_t slot = (head_ + i) % kCapacity;
        char key[kSlotKeyCapacity];
        slotKey(slot, key);
        if (preferences_.getBytes(key, &readings_[slot], sizeof(TelemetryReading)) != sizeof(TelemetryReading)) {
            return false;
        }
    }
    return true;
}

void ReadingBuffer::saveCounters() {
    preferences_.putUInt(kStateKey, static_cast<uint32_t>(head_) << 16 | static_cast<uint32_t>(count_));
}
