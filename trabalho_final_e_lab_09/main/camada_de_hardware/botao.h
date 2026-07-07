#ifndef BOTAO_H
#define BOTAO_H

/**
 * @brief Configura o pino GPIO associado ao botão de chute.
 */
void botao_chute_init(void);

/**
 * @brief Verifica o estado atual do botão.
 * * @return 1 se o botão estiver pressionado, 0 caso contrário.
 */
int botao_chute_is_pressed(void);

#endif // BOTAO_H