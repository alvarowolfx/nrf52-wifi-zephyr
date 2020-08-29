#ifndef __COAP_H__
#define __COAP_H__

#include <net/net_ip.h>
#include <net/socket.h>
#include <net/udp.h>
#include <net/coap.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <kernel.h>

#include "rgb_led.h"

#define MAX_COAP_MSG_LEN 256

typedef enum coap_format_num
{
  /* CBor specific code */
  // https://tools.ietf.org/html/rfc7049#section-7.4
  COAP_FORMAT_APPLICATION_CBOR = 60,
  /* Initial known types */
  // https://tools.ietf.org/html/rfc7252#section-12.3
  COAP_FORMAT_TEXT_PLAIN = 0,
  COAP_FORMAT_APPLICATION_JSON = 50,
} coap_format_t;

bool coap_is_connected();
int coap_connect(const char *server, int port);
void coap_disconnect();
//int coap_send(u8_t method, u8_t *path, u8_t *payload, u16_t payload_len);
int coap_send(u8_t method, u8_t *path, coap_format_t format_num, u8_t *payload, u16_t payload_len);

#endif