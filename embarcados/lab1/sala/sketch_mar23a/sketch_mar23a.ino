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


int main() {

    pinMode(A0, OUTPUT);

    double x;
    double y;
    double incremento;
    int contador;
    int limite = 0;


    while(limite < 11){    
      incremento = (2.0 * PI) / NUM_PONTOS;

      for (contador = 0; contador < NUM_PONTOS; contador++) {
          x = contador * incremento;
          y = meu_seno(x);
          if(y>=0){
            digitalWrite(A0, HIGH);
          }
          else{
            digitalWrite(A0, LOW);
          }
      }
      limite= limite +1;
    }
    
    return 0;
}