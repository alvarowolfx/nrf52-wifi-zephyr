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
#include <drivers/hwinfo.h>
#include <tinycbor/cbor.h>

#include <net/coap.h>

#include "rgb_led.h"
#include "sensor.h"
#include "wifi.h"
#include "coap.h"

LOG_MODULE_REGISTER(main);
#define SLEEP_TIME K_SECONDS(1)

#define PEER_PORT 5683
#define SERVER_ADDR "192.168.86.146"

u8_t dev_id[16];

u8_t cbor_buf[MAX_COAP_MSG_LEN];
int cbor_len;

int cbor_write(struct cbor_encoder_writer *cew, const char *data, int len)
{
  memcpy(cbor_buf + cbor_len, data, len);
  cbor_len += len;

  assert(cbor_len < sizeof(cbor_buf));
  return 0;
}
struct cbor_encoder_writer cbor_writer = {
    .write = cbor_write,
};

void device_id_init()
{
  char dev_str[33];
  ssize_t length;
  int i;

  (void)memset(dev_id, 0x0, sizeof(dev_id));

  length = hwinfo_get_device_id(dev_id, sizeof(dev_id));

  /* If this fails for some reason, use all zeros instead */
  if (length <= 0)
  {
    length = sizeof(dev_id);
  }

  /* Render the obtained serial number in hexadecimal representation */
  for (i = 0; i < length; i++)
  {
    sprintf(&dev_str[i * 2], "%02x", dev_id[i]);
  }

  LOG_INF("Device id: %s", log_strdup(dev_str));
}

void main(void)
{
  LOG_INF("INIT \n");
  device_id_init();
  rgb_led_init();
  rgb_led_set(0xff, 0x00, 0x00);

  wifi_init();

  int transmit_failures = 0;
  while (1)
  {
    if (wifi_is_connected())
    {
      if (!coap_is_connected())
      {
        LOG_INF("CoAP not connected, trying to connect.");
        coap_connect(SERVER_ADDR, PEER_PORT);
      }
      if (coap_is_connected())
      {
        LOG_INF("CoAP connected, trying to send data.");
        int r;
        bool transmit_error = false;
        u8_t temp_buf[10];
        struct sensor_value temp = get_temperature();
        u8_t press_buf[10];
        struct sensor_value press = get_pressure();
        sprintf(temp_buf, "%d.%06d", temp.val1, temp.val2);
        sprintf(press_buf, "%d.%06d", press.val1, press.val2);
        //LOG_INF("Temp %s", log_strdup(temp_buf));
        //LOG_INF("Pres %s", log_strdup(press_buf));

        u8_t temp_uri[32];
        sprintf(temp_uri, "d/%s/s/temp", dev_id);
        u8_t press_uri[32];
        sprintf(press_uri, "d/%s/s/pressure", dev_id);

        r = coap_send(COAP_METHOD_POST, temp_uri, COAP_FORMAT_TEXT_PLAIN, temp_buf, 9);
        if (r < 0)
        {
          transmit_error = true;
          transmit_failures++;
        }
        r = coap_send(COAP_METHOD_POST, press_uri, COAP_FORMAT_TEXT_PLAIN, press_buf, 9);
        if (r < 0)
        {
          transmit_error = true;
          transmit_failures++;
        }

        cbor_len = 0;
        CborEncoder encoder, mapEncoder;
        cbor_encoder_init(&encoder, &cbor_writer, 0);
        cbor_encoder_create_map(&encoder, &mapEncoder, 2);
        cbor_encode_text_stringz(&mapEncoder, "temp");
        //cbor_encode_double(&mapEncoder, sensor_value_to_double(&temp));
        cbor_encode_int(&mapEncoder, temp.val1);
        cbor_encode_text_stringz(&mapEncoder, "pressure");
        //cbor_encode_double(&mapEncoder, sensor_value_to_double(&press));
        cbor_encode_int(&mapEncoder, press.val1);
        cbor_encoder_close_container(&encoder, &mapEncoder);

        u8_t state_uri[32];
        sprintf(state_uri, "d/%s/s/environment", dev_id);
        // LOG_INF("Sending cbor %s - %d", log_strdup(cbor_buf), cbor_len);
        r = coap_send(COAP_METHOD_POST, state_uri, COAP_FORMAT_APPLICATION_CBOR, cbor_buf, cbor_len);
        if (r < 0)
        {
          transmit_error = true;
          transmit_failures++;
        }

        if (!transmit_error)
        {
          transmit_failures = 0;
        }

        if (transmit_failures > 20)
        {
          LOG_ERR("Too many failures, trying to restart network.");
          wifi_disconnect();
          transmit_failures = 0;
        }
        // LOG_INF("Failures %d", transmit_failures);
      }
    }
    else
    {
      LOG_INF("Not connected");
    }

    k_sleep(K_SECONDS(5));
  }
}