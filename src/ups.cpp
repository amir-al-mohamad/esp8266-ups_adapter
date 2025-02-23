#include <math.h>
#include <SoftwareSerial.h>
#include <ups.h>
#include <watchPower.h>

UPS::UPS(SoftwareSerial &_refSer) : WatchPower(_refSer) {}

UPS::~UPS() {}

void UPS::_onConnect() {
  if (_onConnectCallback && !_firstRun) {
    _onConnectCallback();
  }
}

void UPS::_onDisconnect() {
  if (_onDisconnectCallback && !_firstRun) {
    _onDisconnectCallback();
  }
}

void UPS::_onGridConnect() {
  if (_onGridConnectCallback && !_firstRun) {
    _onGridConnectCallback();
  }
}

void UPS::_onGridDisconnect() {
  if (_onGridDisconnectCallback && !_firstRun) {
    _onGridDisconnectCallback();
  }
}

void UPS::_onBatteryCharged(float capacity) {
  if (_onBatteryChargedCallback && !_firstRun) {
    _onBatteryChargedCallback(capacity);
  }
}

void UPS::_onBatteryDischarged(float capacity) {
  if (_onBatteryDischargedCallback && !_firstRun) {
    _onBatteryDischargedCallback(capacity);
  }
}

void UPS::tick() {
  const bool isConnected = refreshData();

  if (_connectionStatus != isConnected) {
    if (isConnected && mode == 'N') {
      _connectionStatus = isConnected;
      
      refreshDeviceConstants();
      refreshSettings();
      _onConnect();

      _firstRun = true;
    } else if (!isConnected) {
      _connectionStatus = isConnected;
      _onDisconnect();
    }
  }

  if (!isConnected || mode != 'N') {
    return;
  }

  const bool isCurrentlyOnGrid = isGridAvailable();

  if (isCurrentlyOnGrid != _gridConnected) {
    _gridConnected = isCurrentlyOnGrid;

    if (isCurrentlyOnGrid) {
      _onGridConnect();
    } else {
      _onGridDisconnect();
    }
  }

  const int batteryLevel = round(batteryCapacity.flt * 10);

  if (isCurrentlyOnGrid && batteryLevel > 90 && !_batteryCharged) {
    _batteryCharged = true;
    _onBatteryCharged(batteryLevel);
  }

  if (!isCurrentlyOnGrid && batteryLevel < 10 && _batteryCharged) {
    _batteryCharged = false;
    _onBatteryDischarged(batteryLevel);
  }

  _firstRun = false;
}

void UPS::onConnect(void (*callback)()) {
  _onConnectCallback = callback;
}

void UPS::onDisconnect(void (*callback)()) {
  _onDisconnectCallback = callback;
}

void UPS::onGridConnect(void (*callback)()) {
  _onGridConnectCallback = callback;
}

void UPS::onGridDisconnect(void (*callback)()) {
  _onGridDisconnectCallback = callback;
}

void UPS::onBatteryCharged(void (*callback)(float)) {
  _onBatteryChargedCallback = callback;
}

void UPS::onBatteryDischarged(void (*callback)(float)) {
  _onBatteryDischargedCallback = callback;
}

UPSStatus UPS::getStatus() {
  UPSStatus data;
  data.isConnected = mode == 'N';
  data.load.percent = round(loadPercent.flt);
  data.load.watt = round(outputPowerActive.flt);
  data.battery.capacity = round(batteryCapacity.flt * 10);
  data.battery.voltage = round(batteryVoltage.flt * 10.0f) / 10.0f;
  data.temperature = round(temperature.flt * 10);
  data.grid.voltage = round(gridVoltage.flt);
  data.grid.frequency = round(gridFreq.flt * 10.0f) / 10.0f;
  data.grid.isConnected = isGridAvailable();
  data.output.voltage = round(outputVoltage.flt);
  data.output.frequency = round(outputFreq.flt * 10.0f) / 10.0f;
  return data;
}
