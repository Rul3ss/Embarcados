#ifndef BOTAO_H
#define BOTAO_H

/**
 * @brief Configura o GPIO e a interrupcao associada ao botao de chute.
 */
void botao_chute_init(void);

/**
 * @brief Retorna o ultimo estado do botao atualizado pela interrupcao.
 *
 * @return 1 se o botao estiver pressionado, 0 caso contrario.
 */
int botao_chute_is_pressed(void);

#endif
