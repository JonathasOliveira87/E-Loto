#include "Estado.h"

Trabalhador cadastroTrabalhadores[LIMITE_TRABALHADORES] = {};

const char *NOMES_EXEMPLO[] = {
    "NICHOLAS MARIANO", "JONATHAS OLIVEIRA", "LORENZO MARIANO", "DRIELLE ALESSANDRA"
};

const char *FUNCOES_EXEMPLO[] = {
    "MECANICO", "AJUDANTE BOB MP6", "SOLDADOR", "OPERADORA"
};

const int QUANTIDADE_EXEMPLOS = 4;

int quantidadeTrabalhadoresAtivos = 0;
int quantidadeTrabalhadoresCadastrados = 0;
bool equipamentoBloqueado = false;
Tela telaAtual = TELA_INICIAL;

uint32_t ultimoToqueMillis = 0;
