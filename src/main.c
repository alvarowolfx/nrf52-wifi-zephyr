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

#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/udp.h>
#include <net/coap.h>
#include <net/wifi_mgmt.h>
#include <net/net_event.h>

LOG_MODULE_REGISTER(main);

#include "rgb_led.h"

#define SLEEP_TIME K_SECONDS(1)

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | \
                                NET_EVENT_WIFI_DISCONNECT_RESULT)

/* CoAP socket fd */
#define PEER_PORT 5683
#define MAX_COAP_MSG_LEN 256
#define SERVER_ADDR "192.168.87.21"
static int sock;
struct pollfd fds[1];
static int nfds;
static const char *const path[] = {"/state", NULL};

static struct net_mgmt_event_callback wifi_mgmt_cb;

static struct wifi_connect_req_params cnx_params =
    {
        .ssid = "FullStackIoT",
        .ssid_length = 12,
        .psk = "FullStackIoT",
        .psk_length = 12,
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_PSK,
};

static void prepare_fds(void)
{
  fds[nfds].fd = sock;
  fds[nfds].events = POLLIN;
  nfds++;
}

static int start_coap_client(void)
{
  int ret = 0;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(PEER_PORT);

  inet_pton(AF_INET, SERVER_ADDR,
            &addr.sin_addr);

  sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
  {
    LOG_ERR("Failed to create UDP socket %d", errno);
    return -errno;
  }

  ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
  {
    LOG_ERR("Cannot connect to UDP remote : %d", errno);
    return -errno;
  }

  prepare_fds();

  return 0;
}

static int send_simple_coap_request(u8_t method)
{
  u8_t payload[] = "payload";
  struct coap_packet request;
  const char *const *p;
  u8_t *data;
  int r;

  data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
  if (!data)
  {
    return -ENOMEM;
  }

  r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
                       1, COAP_TYPE_CON, 8, coap_next_token(),
                       method, coap_next_id());
  if (r < 0)
  {
    LOG_ERR("Failed to init CoAP message");
    goto end;
  }

  for (p = path; p && *p; p++)
  {
    r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                  *p, strlen(*p));
    if (r < 0)
    {
      LOG_ERR("Unable add option to request");
      goto end;
    }
  }

  switch (method)
  {
  case COAP_METHOD_GET:
  case COAP_METHOD_DELETE:
    break;

  case COAP_METHOD_PUT:
  case COAP_METHOD_POST:
    r = coap_packet_append_payload_marker(&request);
    if (r < 0)
    {
      LOG_ERR("Unable to append payload marker");
      goto end;
    }

    r = coap_packet_append_payload(&request, (u8_t *)payload,
                                   sizeof(payload) - 1);
    if (r < 0)
    {
      LOG_ERR("Not able to append payload");
      goto end;
    }

    break;
  default:
    r = -EINVAL;
    goto end;
  }

  //net_hexdump("Request", request.data, request.offset);

  r = send(sock, request.data, request.offset, 0);

end:
  k_free(data);

  return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
  ARG_UNUSED(cb);

  LOG_INF("Connected\n");
  rgb_led_set(0x00, 0x7f, 0x00);
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
  ARG_UNUSED(cb);

  LOG_INF("Disconnected\n");
  rgb_led_set(0x7f, 0x00, 0x00);
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

int send_request()
{
  int r;
  r = start_coap_client();
  if (r < 0)
  {
    (void)close(sock);
    return r;
  }

  r = send_simple_coap_request(COAP_METHOD_GET);
  if (r < 0)
  {
    return r;
  }

  return 0;
}

void main(void)
{
  //log_init();

  printk("INIT \n");
  LOG_INF("INIT \n");
  rgb_led_init();
  rgb_led_set(0xff, 0x00, 0x00);

  LOG_INF("SET LEDS \n");

  k_sleep(SLEEP_TIME);
  rgb_led_set(0x00, 0x00, 0xFF);
  k_sleep(SLEEP_TIME);
  rgb_led_set(0x00, 0xFF, 0x00);
  k_sleep(SLEEP_TIME);
  rgb_led_set(0xFF, 0x00, 0x00);

  net_mgmt_init_event_callback(&wifi_mgmt_cb,
                               wifi_mgmt_event_handler,
                               WIFI_SHELL_MGMT_EVENTS);

  net_mgmt_add_event_callback(&wifi_mgmt_cb);

  struct net_if *iface = net_if_get_default();

  if (net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface, NULL, 0))
  {
    rgb_led_set(0xFF, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
    rgb_led_set(0xFF, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
    rgb_led_set(0xFF, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
  }

  LOG_INF("NET MGMT");

  if (!net_if_is_up(iface))
  {
    rgb_led_set(0x7f, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
    rgb_led_set(0x7f, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
    rgb_led_set(0x7f, 0x00, 0x00);
    k_sleep(SLEEP_TIME);
  }

  rgb_led_set(0x7f, 0x00, 0x00);

  int cnt = 0;
  while (1)
  {
    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
             &cnx_params, sizeof(struct wifi_connect_req_params));
    send_request();
    k_sleep(K_SECONDS(10));
    cnt++;
    LOG_INF("Hello %d \n", cnt);
  }
}