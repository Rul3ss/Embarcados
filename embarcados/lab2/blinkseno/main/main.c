#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_PIN 2

#define PI 3.14159265358979323846
#define NUM_PONTOS 1000
#define MAX_TERMOS 10

double fatorial(int n) {
    double resultado = 1.0;
    int i;
    
    for (i = 2; i <= n; i++) {
        resultado = resultado * i;
    }
    
    return resultado;
}

double potencia(double base, int expoente) {
    double resultado = 1.0;
    int i;
    
    for (i = 0; i < expoente; i++) {
        resultado = resultado * base;
    }
    
    return resultado;
}


double meu_seno(double x) {
    double resultado = 0.0;
    int n;
    int sinal;
    

    while (x > 2 * PI) {
        x -= 2 * PI;
    }
    while (x < -2 * PI) {
        x += 2 * PI;
    }
    

    for (n = 0; n < MAX_TERMOS; n++) {
        int expoente = 2 * n + 1;
        

        sinal = (n % 2 == 0) ? 1 : -1;
        
        resultado += sinal * potencia(x, expoente) / fatorial(expoente);
    }
    
    return resultado;
}

void app_main(void)
{


    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    double x;
    double y;
    double incremento;
    int contador;

    incremento = (2.0 * PI) / NUM_PONTOS;

    while(1){

     for (contador = 0; contador < NUM_PONTOS; contador++) {
        x = contador * incremento;
        y = meu_seno(x);
        if(y<0){  
          gpio_set_level(LED_PIN, true);
        }else{
          gpio_set_level(LED_PIN, false);
        }
        //vTaskDelay(pdMS_TO_TICKS(10)); testar pra ver se vai precisar do delay
       
    }
    }


}
