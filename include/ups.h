#include <SoftwareSerial.h>
#include <watchPower.h>

#ifndef INVERTER_TYPES_H
#define INVERTER_TYPES_H

struct UPSLoadStatus {
  int percent;
  int watt;
};

struct UPSBatteryStatus {
  int capacity;
  float voltage;
};

struct UPSOutputStatus {
  int voltage;
  float frequency;
};

struct UPSGridStatus : UPSOutputStatus {
  bool isConnected;
};

struct UPSStatus {
  bool isConnected;
  UPSLoadStatus load;
  UPSBatteryStatus battery;
  int temperature;
  UPSGridStatus grid;
  UPSOutputStatus output;
};

class UPS : public WatchPower {
  private:
    void (*_onConnectCallback)() = nullptr;
    void (*_onDisconnectCallback)() = nullptr;
    void (*_onGridConnectCallback)() = nullptr;
    void (*_onGridDisconnectCallback)() = nullptr;
    void (*_onBatteryDischargedCallback)(float) = nullptr;
    void (*_onBatteryChargedCallback)(float) = nullptr;

    bool _connectionStatus = false;
    bool _firstRun = true;
    bool _gridConnected = false;
    bool _batteryCharged = false;

    void _onConnect();
    void _onDisconnect();
    void _onGridConnect();
    void _onGridDisconnect();
    void _onBatteryDischarged(float capacity);
    void _onBatteryCharged(float capacity);

  public:
    UPS(SoftwareSerial &_refSer);
    ~UPS();

    void tick();
    void onConnect(void (*callback)());
    void onDisconnect(void (*callback)());
    void onGridConnect(void (*callback)());
    void onGridDisconnect(void (*callback)());
    void onBatteryCharged(void (*callback)(float));
    void onBatteryDischarged(void (*callback)(float));

    UPSStatus getStatus();
};

#endif
