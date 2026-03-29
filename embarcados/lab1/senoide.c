#include <stdio.h>


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
    double x;
    double y;
    double incremento;
    int contador;
    FILE *arquivo; // Ponteiro para o arquivo de saída

    
    incremento = (2.0 * PI) / NUM_PONTOS;
    printf("%f\n", incremento);

    // Abre o arquivo para escrita
    arquivo = fopen("senoide.txt", "w");
    
    // Verifica se o arquivo foi aberto com sucesso
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo!\n");
        return 1;
    }

    for (contador = 0; contador < NUM_PONTOS; contador++) {
        x = contador * incremento;
        y = meu_seno(x);
        printf("Ponto %4d: x = %.6f, sen(x) = %.6f\n", contador, x, y);
        
        // Salva os valores x e y no arquivo, separados por espaço
        fprintf(arquivo, "%.6f %.6f\n", x, y);
    }

    // Fecha o arquivo
    fclose(arquivo);
    
    printf("\nDados salvos em 'senoide.txt'\n");
    
    return 0;
}