#include <zephyr.h>
#include <device.h>
#include <drivers/pwm.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(ble_rgb_light_led, LOG_LEVEL_DBG);

#define PERIOD (USEC_PER_SEC / 100)
#define PWM0_NODE DT_NODELABEL(pwm0)
#define PWM_DEV DT_LABEL(PWM0_NODE)
#define RED_PIN DT_PROP(PWM0_NODE, ch0_pin)
#define GREEN_PIN DT_PROP(PWM0_NODE, ch1_pin)
#define BLUE_PIN DT_PROP(PWM0_NODE, ch2_pin)

struct device *pwm_dev;

void rgb_led_init()
{

  pwm_dev = device_get_binding(PWM_DEV);

  if (!pwm_dev)
  {
    LOG_ERR("Cannot find PWM device!\n");
    return;
  }
}

void rgb_led_set(u32_t r, u32_t g, u32_t b)
{
  if (!pwm_dev)
  {
    return;
  }
  pwm_pin_set_usec(pwm_dev, RED_PIN, PERIOD, ((255 - (r & 0xff)) * PERIOD) >> 8, PWM_POLARITY_NORMAL);
  k_sleep(K_MSEC(10));
  pwm_pin_set_usec(pwm_dev, GREEN_PIN, PERIOD, ((255 - (g & 0xff)) * PERIOD) >> 8, PWM_POLARITY_NORMAL);
  k_sleep(K_MSEC(10));
  pwm_pin_set_usec(pwm_dev, BLUE_PIN, PERIOD, ((255 - (b & 0xff)) * PERIOD) >> 8, PWM_POLARITY_NORMAL);
  k_sleep(K_MSEC(10));
}