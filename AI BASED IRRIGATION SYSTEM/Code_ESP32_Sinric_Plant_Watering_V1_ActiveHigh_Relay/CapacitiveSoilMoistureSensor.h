#ifndef _CAPACITIVESOILMOISTURESENSOR_H_
#define _CAPACITIVESOILMOISTURESENSOR_H_

#include <SinricProDevice.h>
#include <Capabilities/ModeController.h>
#include <Capabilities/RangeController.h>
#include <Capabilities/PushNotification.h>

class CapacitiveSoilMoistureSensor 
: public SinricProDevice
, public ModeController<CapacitiveSoilMoistureSensor>
, public RangeController<CapacitiveSoilMoistureSensor>
, public PushNotification<CapacitiveSoilMoistureSensor> {
  friend class ModeController<CapacitiveSoilMoistureSensor>;
  friend class RangeController<CapacitiveSoilMoistureSensor>;
  friend class PushNotification<CapacitiveSoilMoistureSensor>;
public:
  CapacitiveSoilMoistureSensor(const String &deviceId) : SinricProDevice(deviceId, "CapacitiveSoilMoistureSensor") {};
};

#endif