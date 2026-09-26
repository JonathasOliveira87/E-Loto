#pragma once
#include "Config.h"

void mostrarTelaInicial();
void mostrarTrabalhadores();
void mostrarBloqueio();
void mostrarMais();
void mostrarHistorico();
void mostrarWiFi();
void carregarTela(Tela novaTela);
void abrirModalTrabalhador(int indice);

void mostrarLoadingRFID(const char *texto);
void fecharLoadingRFID();
void mostrarMensagemRFID(const char *titulo, const char *nome, const char *subtitulo, lv_color_t cor);
bool mostrarConfirmacaoRemocaoRFID(int indice);
void fecharConfirmacaoRFID();
void mostrarConfirmacaoApagarLogs();

void atualizarStatusWiFiTela();
