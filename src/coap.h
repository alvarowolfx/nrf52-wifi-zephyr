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

bool coap_is_connected();
int coap_connect(const char *server, int port);
void coap_disconnect();
int coap_send(u8_t method, u8_t *path, u8_t *payload, u16_t payload_len);

#endif