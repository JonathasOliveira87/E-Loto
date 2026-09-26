#pragma once
#include <stdint.h>
#include <stddef.h>

void iniciarPN532();
void lerRFID();
void uidParaTexto(uint8_t *uid, uint8_t uidLength, char *saida, size_t tamanho);
int encontrarTrabalhadorRFID(const char *uidTexto);
int encontrarTrabalhadorCadastrado(const char *uidTexto);
void cadastrarTrabalhadorRFID(const char *uidTexto);
void ativarTrabalhador(int indice);
void desativarTrabalhador(int indice);

void confirmarRemocaoRFID(int indice);
void cancelarRemocaoRFID();

// Estatisticas do tempo de resposta do leitor RFID (entre a deteccao
// do cartao e a conclusao do registro), calculadas em tempo real no
// firmware e expostas no /estado.json. Nao ha tela local para isso -
// os valores sao consultados pelo dashboard web, para nao gastar RAM
// do ESP32 com uma interface extra.
extern uint32_t rfidQuantidadeLeituras;
extern uint32_t rfidUltimoTempoRespostaMs;
extern float rfidTempoRespostaMedioMs;
extern float rfidTempoRespostaDesvioMs;

// Janela deslizante com as ultimas leituras (bancada de testes), so
// para preencher a tabela do TCC - nao e persistida em lugar nenhum.
extern uint16_t rfidUltimasLeiturasMs[30];
extern uint8_t rfidUltimasLeiturasCount;
size_t copiarUltimasLeiturasRFID(uint16_t *destino, size_t max);
