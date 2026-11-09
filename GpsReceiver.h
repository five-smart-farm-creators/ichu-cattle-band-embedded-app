#ifndef GPS_RECEIVER_H
#define GPS_RECEIVER_H

#include <time.h>

#include <TinyGPS++.h>

// NEO-6M on UART1, parsed by TinyGPS++; poll() must run every loop or sentences are lost.
class GpsReceiver {
public:
    void begin();
    void poll();
    // TinyGPS++ accessors are not const, so these cannot be.
    bool position(double& latitude, double& longitude, float& accuracyMeters);
    bool utcDateTime(tm& utc);

private:
    TinyGPSPlus gps_;
    bool silenceReported_ = false;
};

#endif  // GPS_RECEIVER_H
