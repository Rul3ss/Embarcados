idf_component_register(SRCS "main.c"
                            "camada_de_hardware/botao.c"
                            "camada_de_hardware/joystick.c"
                            "camada_de_sistema/wifi.c"
                            "camada_de_sistema/udp_cliente.c"
                    INCLUDE_DIRS "."
                                 "camada_de_hardware"
                                 "camada_de_sistema")