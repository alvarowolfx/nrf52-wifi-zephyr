#include "coap.h"

LOG_MODULE_REGISTER(coap);

/* CoAP socket fd */
int sock;
#define MAX_FDS 2
struct pollfd fds[MAX_FDS];
int nfds = 0;
//const char *const path[] = {"/state", NULL};

int add_fds(void)
{
  if (nfds < MAX_FDS)
  {
    fds[nfds].fd = sock;
    fds[nfds].events = POLLIN;
    nfds++;
    return 0;
  }
  else
  {
    for (int i = 0; i < MAX_FDS; i++)
    {
      if (fds[i].fd < 0)
      {
        fds[i].fd = sock;
        fds[i].events = POLLIN;
        return 0;
      }
    }
  }
  return -1;
}

int wait(void)
{
  int ret;
  ret = poll(fds, nfds, 1000);
  if (ret < 0)
  {
    LOG_ERR("Error in poll:%d", errno);
  }
  return ret;
}

int coap_connect(const char *server_addr, int port)
{
  int ret = 0;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  inet_pton(AF_INET, server_addr,
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

  ret = add_fds();

  return ret;
}

int send_coap_request(u8_t method, u8_t *path, u8_t *payload, u16_t payload_len)
{
  struct coap_packet request;
  u8_t *data;
  int r;

  data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
  if (!data)
  {
    LOG_ERR("No memory to alloc coap msg");
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

  r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                path, strlen(path));
  if (r < 0)
  {
    LOG_ERR("Unable add option to request");
    goto end;
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
                                   payload_len);
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
  if (r < 0)
  {
    LOG_ERR("Not able to send data");
  }

end:
  k_free(data);

  return r;
}

int process_coap_reply(void)
{
  struct coap_packet reply;
  uint8_t *data;
  int rcvd;
  int ret;

  ret = wait();
  if (ret < 0)
  {
    return ret;
  }

  data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
  if (!data)
  {
    return -ENOMEM;
  }

  rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);

  if (rcvd == 0)
  {
    ret = -EIO;
    goto end;
  }

  if (rcvd < 0)
  {
    ret = -errno;
    goto end;
  }

  // net_hexdump("Response", data, rcvd);

  ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
  if (ret < 0)
  {
    LOG_ERR("Invalid data received");
    goto end;
  }

  u16_t payload_len = -1;
  u8_t *payload = coap_packet_get_payload(&reply, &payload_len);

  if (payload_len <= 0)
  {
    LOG_INF("No payload");
    goto end;
  }
  payload[payload_len] = '\0';

  LOG_INF("REPLY %s", log_strdup(payload));

end:
  k_free(data);

  return ret;
}

bool coap_is_connected()
{
  return sock >= 0;
}

void coap_disconnect()
{
  for (int i = 0; i < MAX_FDS; i++)
  {
    if (fds[i].fd == sock)
    {
      fds[i].fd = -1;
    }
  }
  (void)close(sock);
  sock = -1;
}

int coap_send(u8_t method, u8_t *path, u8_t *payload, u16_t payload_len)
{
  int r;
  r = send_coap_request(method, path, payload, payload_len);
  if (r < 0)
  {
    goto end;
  }

  r = process_coap_reply();
  if (r < 0)
  {
    goto end;
  }

  LOG_INF("Sent CoAP request sucessfully");

end:
  if (r < 0)
  {
    coap_disconnect();
  }

  return r;
}