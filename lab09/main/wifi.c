


#include "wifi.h"


const char *ssid = "203";
const char *pass = "pacoquita";

static int retry_num = 0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
if(event_id == WIFI_EVENT_STA_START)
{
  printf("WIFI CONNECTING....\n");
}
else if (event_id == WIFI_EVENT_STA_CONNECTED)
{
  printf("WiFi CONNECTED\n");
}
else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
  printf("WiFi lost connection\n");
  if(retry_num<5){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
}
else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

        printf("WiFi conectado!\n");
        printf("IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));

        retry_num = 0;
}
}
void wifi_connection(){
esp_netif_init(); //network interdace initialization
esp_event_loop_create_default(); //responsible for handling and dispatching events
esp_netif_create_default_wifi_sta(); //sets up necessary data structs for wifi station interface
wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();//sets up wifi wifi_init_config struct with default values
esp_wifi_init(&wifi_initiation); //wifi initialised with dafault wifi_initiation
esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);//creating event handler register for wifi
esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);//creating event handler register for ip event
wifi_config_t wifi_configuration ={ //struct wifi_config_t var wifi_configuration
.sta= {
    .ssid = "203",
    .password= "pacoquita", /*we are sending a const char of ssid and password which we will strcpy in following line so leaving it blank*/ 
  }//also this part is used if you donot want to use Kconfig.projbuild
};
strcpy((char*)wifi_configuration.sta.ssid,ssid); // copy chars from hardcoded configs to struct
strcpy((char*)wifi_configuration.sta.password,pass);
esp_wifi_set_mode(WIFI_MODE_STA);//station mode selected
esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);//setting up configs when event ESP_IF_WIFI_STA
esp_wifi_start();//start connection with configurations provided in funtion
esp_wifi_connect(); //connect with saved ssid and pass
printf( "wifi_init_softap finished. SSID:%s  password:%s",ssid,pass);
}
