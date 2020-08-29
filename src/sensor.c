#include "sensor.h"

LOG_MODULE_REGISTER(sensor);

#define BME280 DT_INST(0, bosch_bme280)

#if DT_NODE_HAS_STATUS(BME280, okay)
#define BME280_LABEL DT_LABEL(BME280)
#else
#error Your devicetree has no enabled nodes with compatible "bosch,bme280"
#define BME280_LABEL "<none>"
#endif

#define BME280_BUS_I2C DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

struct sensor_value temp, press, humidity;

struct sensor_value get_temperature()
{
  return temp;
}

struct sensor_value get_pressure()
{
  return press;
}

struct sensor_value get_humidity()
{
  return humidity;
}

void sensor_task(void)
{
  struct device *dev = device_get_binding(BME280_LABEL);
  if (dev == NULL)
  {
    LOG_ERR("No device \"%s\" found; did initialization fail?",
            BME280_LABEL);
    return;
  }
  else
  {
    LOG_INF("Found device \"%s\"", BME280_LABEL);
  }

  while (1)
  {
    sensor_sample_fetch(dev);
    sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
    sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

    LOG_DBG("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d",
            temp.val1, temp.val2, press.val1, press.val2,
            humidity.val1, humidity.val2);

    k_sleep(K_MSEC(300));
  }
}

K_THREAD_DEFINE(sensor_task_id, 1024, sensor_task, NULL, NULL, NULL, 7, 0, 100);