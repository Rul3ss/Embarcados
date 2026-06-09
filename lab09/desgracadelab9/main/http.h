#ifndef HTTP_H
#define HTTP_H

#include "esp_http_client.h"



static esp_err_t http_event_handler(esp_http_client_event_t *evt);

void fazer_get_ping(void);
void fazer_post_dados(void);


#endif