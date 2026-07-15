#ifndef JOYSTICK_H
#define JOYSTICK_H

/**
 * @brief Configura o ADC, calibra o centro e inicia a leitura continua.
 */
void joystick_init(void);

/**
 * @brief Retorna a ultima amostra do joystick atualizada pelo ADC continuo.
 *
 * @param x Ponteiro onde sera salvo o valor de X normalizado (-1.0 a 1.0).
 * @param y Ponteiro onde sera salvo o valor de Y normalizado (-1.0 a 1.0).
 */
void joystick_read_normalized(float *x, float *y);

#endif
