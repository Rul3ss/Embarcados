#ifndef WIFI_H
#define WIFI_H

#include <stdio.h> 
#include <string.h> 
#include "freertos/FreeRTOS.h" 
#include "freertos/event_groups.h" // Adicionado para gerenciar os eventos
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_log.h" 
#include "esp_event.h" 
#include "nvs_flash.h" 
#include "lwip/err.h" 
#include "lwip/sys.h" 

// Definimos o bit de conexão e expomos o grupo de eventos para a main.c
#define WIFI_CONNECTED_BIT BIT0
extern EventGroupHandle_t s_event_group;

void wifi_connection(void);

#endif