#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <stdbool.h>

/**
 * @brief Inicializa o socket UDP, conecta ao servidor e realiza o handshake.
 * @return true se o handshake for bem-sucedido, false caso contrário.
 */
bool udp_init_and_handshake(void);

/**
 * @brief Envia os dados de entrada do jogador para o servidor.
 * * @param jx Valor normalizado do eixo X do joystick.
 * @param jy Valor normalizado do eixo Y do joystick.
 * @param kick Estado do botão de chute (1 = pressionado, 0 = solto).
 */
void udp_send_player_input(float jx, float jy, int kick);

/**
 * @brief Verifica se há mensagens recebidas do servidor (não-bloqueante).
 */
void udp_receive_server_data(void);

#endif