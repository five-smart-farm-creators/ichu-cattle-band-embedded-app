#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <stdint.h>

// MQ-2 analog output mapped to a 0-100 relative methane index; the module is neither
// calibrated nor selective, so the index tracks change, not ppm.
class GasSensor {
public:
    void begin(uint32_t nowMs);
    // The heater needs time after power-up before the output settles.
    bool isWarmingUp(uint32_t nowMs) const;
    float methaneIndex() const;

private:
    uint32_t poweredAtMs_ = 0;
};

#endif  // GAS_SENSOR_H
