#include "http.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG_HTTP = "HTTP_CLIENT";

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG_HTTP, "Conectado ao servidor");
        break;

    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG_HTTP, "Resposta do servidor:");
        printf("%.*s\n", evt->data_len, (char *)evt->data);
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG_HTTP, "Requisicao finalizada");
        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_HTTP, "Desconectado do servidor");
        break;

    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG_HTTP, "Erro HTTP");
        break;

    default:
        break;
    }

    return ESP_OK;
}
void fazer_get_ping(void)
{
    esp_http_client_config_t config = {
        .url = "http://10.110.248.91:3000/ping",
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .keep_alive_enable = false,  // desliga keep-alive
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG_HTTP, "Erro ao criar cliente HTTP");
        return;
    }

    esp_http_client_set_header(client, "Connection", "close");

    int64_t inicio = esp_timer_get_time();

    esp_err_t err = esp_http_client_perform(client);

    int64_t fim = esp_timer_get_time();
    int64_t tempo_ms = (fim - inicio) / 1000;

    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);

        ESP_LOGI(TAG_HTTP, "GET enviado com sucesso");
        ESP_LOGI(TAG_HTTP, "Status HTTP: %d", status);
        ESP_LOGI(TAG_HTTP, "Tempo de resposta: %lld ms", tempo_ms);
    }
    else
    {
        ESP_LOGE(TAG_HTTP, "Erro no GET: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void fazer_post_dados(void)
{
    // 1. Apontamos para a rota '/dados' do servidor e mudamos o método para POST
    esp_http_client_config_t config = {
        .url = "http://10.110.248.91:3000/dados",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .buffer_size = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL) {
        ESP_LOGE(TAG_HTTP, "Erro ao criar cliente HTTP");
        return;
    }

    // 2. Definimos o cabeçalho Content-Type para avisar o Express que é um JSON
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // 3. Criamos a string JSON com as leituras que queremos enviar
    // Dica: Em um caso real, você pode usar sprintf() ou cJSON para montar essa string dinamicamente
    const char *json_dados = "{\"temperatura\":24.8,\"umidade\":62}";
    
    // Anexa o JSON ao corpo da requisição
    esp_http_client_set_post_field(client, json_dados, strlen(json_dados));

    // Executa a transmissão de forma síncrona
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG_HTTP, "POST enviado com sucesso");
        ESP_LOGI(TAG_HTTP, "Status HTTP retornado: %d", status);
    } else {
        ESP_LOGE(TAG_HTTP, "Erro ao executar o POST: %s", esp_err_to_name(err));
    }

    // Libera a memória alocada pelo cliente
    esp_http_client_cleanup(client);
}