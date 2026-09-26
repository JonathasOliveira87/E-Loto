#pragma once
#include "Config.h"
#include <Arduino.h>

extern Trabalhador cadastroTrabalhadores[LIMITE_TRABALHADORES];
extern int quantidadeTrabalhadoresAtivos;
extern int quantidadeTrabalhadoresCadastrados;
extern bool equipamentoBloqueado;
extern Tela telaAtual;
extern uint32_t ultimoToqueMillis;

extern const char *NOMES_EXEMPLO[];
extern const char *FUNCOES_EXEMPLO[];
extern const int QUANTIDADE_EXEMPLOS;

void atualizarEstadoBloqueio();
void ativarTrabalhador(int indice);
void desativarTrabalhador(int indice);
