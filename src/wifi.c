#include "wifi.h"

LOG_MODULE_REGISTER(wifi);

bool connected = false;
bool connecting = false;
struct net_mgmt_event_callback wifi_mgmt_cb;

struct wifi_connect_req_params cnx_params =
    {
        .ssid = "FullStackIoT",
        .ssid_length = 12,
        .psk = "FullStackIoT",
        .psk_length = 12,
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_PSK,
};

void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
  ARG_UNUSED(cb);

  LOG_INF("Connected\n");
  rgb_led_set(0x00, 0x7f, 0x00);
  connecting = false;
  connected = true;
}

void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
  ARG_UNUSED(cb);

  LOG_INF("Disconnected\n");
  rgb_led_set(0x7f, 0x00, 0x00);
  connecting = false;
  connected = false;
}

static void wifi_mgmt_event_handler(
    struct net_mgmt_event_callback *cb,
    uint32_t mgmt_event,
    struct net_if *iface)
{
  switch (mgmt_event)
  {
  case NET_EVENT_WIFI_CONNECT_RESULT:
    handle_wifi_connect_result(cb);
    break;
  case NET_EVENT_WIFI_DISCONNECT_RESULT:
    handle_wifi_disconnect_result(cb);
    break;
  default:
    break;
  }
}

void wifi_connect()
{
  struct net_if *iface = net_if_get_default();
  net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
           &cnx_params, sizeof(struct wifi_connect_req_params));
  connecting = true;
}

void wifi_disconnect()
{
  // Try to restart ?
  struct net_if *iface = net_if_get_default();
  net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
  connected = false;
  connecting = false;
}

bool wifi_is_connected()
{
  return connected;
}

bool wifi_is_connecting()
{
  return connecting;
}

void wifi_init()
{
  net_mgmt_init_event_callback(&wifi_mgmt_cb,
                               wifi_mgmt_event_handler,
                               WIFI_SHELL_MGMT_EVENTS);

  net_mgmt_add_event_callback(&wifi_mgmt_cb);

  struct net_if *iface = net_if_get_default();

  if (net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0))
  {
    rgb_led_set(0xFF, 0x00, 0x00);
    return;
  }

  if (!net_if_is_up(iface))
  {
    rgb_led_set(0x7f, 0x00, 0x00);
    return;
  }
}

void wifi_task(void)
{
  //struct net_if *iface = net_if_get_default();
  int tries = 0;

  while (1)
  {
    LOG_INF("WiFi status : %s", connected ? "ONLINE" : "OFFLINE");
    if (connecting)
    {
      LOG_INF("Waiting to connect - %d", tries);
      tries++;
      if (tries > 10)
      {
        tries = 0;
        connecting = false;
        connected = false;
      }
    }
    //bool net_interface_connected = net_if_is_up(iface);
    //LOG_INF("Checking Net status : %s", net_interface_connected ? "ONLINE" : "OFFLINE");

    if (!connected && !connecting)
    {
      LOG_INF("Trying to connect");
      wifi_connect();
    }

    k_sleep(K_SECONDS(1));
  }
}

K_THREAD_DEFINE(wifi_task_id, 1024, wifi_task, NULL, NULL, NULL, 7, 0, 100);