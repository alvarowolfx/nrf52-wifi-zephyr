#include <zephyr.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>

#include <net/coap.h>

#include "rgb_led.h"
#include "sensor.h"
#include "wifi.h"
#include "coap.h"

LOG_MODULE_REGISTER(main);
#define SLEEP_TIME K_SECONDS(1)

#define PEER_PORT 5683
#define SERVER_ADDR "192.168.87.22"

void main(void)
{
  LOG_INF("INIT \n");
  rgb_led_init();
  rgb_led_set(0xff, 0x00, 0x00);

  wifi_init();

  rgb_led_set(0x7f, 0x00, 0x00);

  while (1)
  {
    if (wifi_is_connected())
    {
      if (!coap_is_connected())
      {
        coap_connect(SERVER_ADDR, PEER_PORT);
      }
      else
      {
        /*cbor_encoder_writer *writer;
        CborEncoder encoder, mapEncoder;
        cbor_encoder_init(&encoder, writer, 0);
        cbor_encoder_create_map(&encoder, &mapEncoder, 1);
        cbor_encode_text_stringz(&mapEncoder, "t");
        cbor_encode_float(&mapEncoder, get_temperature().val1);
        cbor_encoder_close_container(&encoder, &mapEncoder);*/
        u8_t temp_buf[10];
        struct sensor_value temp = get_temperature();
        u8_t press_buf[10];
        struct sensor_value press = get_pressure();
        sprintf(temp_buf, "%d.%06d", temp.val1, temp.val2);
        sprintf(press_buf, "%d.%06d", press.val1, press.val2);
        //LOG_INF("Temp %s", log_strdup(temp_buf));
        //LOG_INF("Pres %s", log_strdup(press_buf));

        coap_send(COAP_METHOD_POST, "/state/temp", temp_buf, 9);
        coap_send(COAP_METHOD_POST, "/state/pressure", press_buf, 9);
      }
    }
    k_sleep(K_SECONDS(1));
  }
}