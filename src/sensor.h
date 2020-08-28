#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <kernel.h>

struct sensor_value get_temperature();
struct sensor_value get_pressure();
struct sensor_value get_humidity();

#endif