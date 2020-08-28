#ifndef __WIFI_H__
#define __WIFI_H__

#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/net_mgmt.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
#include <kernel.h>

#include "rgb_led.h"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
                                NET_EVENT_WIFI_DISCONNECT_RESULT)

void wifi_init();
void wifi_connect();
bool wifi_is_connected();
bool wifi_is_connecting();

#endif