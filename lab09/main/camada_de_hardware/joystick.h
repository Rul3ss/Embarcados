#ifndef JOYSTICK_H
#define JOYSTICK_H

/**
 * @brief Configura os pinos do ADC e realiza a calibração inicial do centro.
 */
void joystick_init(void);

/**
 * @brief Lê o joystick e retorna os valores nos eixos X e Y.
 * * @param x Ponteiro onde será salvo o valor de X normalizado (-1.0 a 1.0)
 * @param y Ponteiro onde será salvo o valor de Y normalizado (-1.0 a 1.0)
 */
void joystick_read_normalized(float *x, float *y);

#endif // JOYSTICK_H