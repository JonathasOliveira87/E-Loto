#pragma once
#include <Arduino.h>
#include "Config.h"

struct RegistroLog {
    uint32_t id;
    char evento[12];
    char nome[26];
    char funcao[20];
    char uidCartao[24];
    char data[11];
    char hora[9];
    char equipamento[32];
};

// Buffer pequeno em RAM: usado so pela tela de historico local do
// equipamento (mostra os ultimos 8 - ver Telas.cpp). Continua limitado
// a LIMITE_LOGS por causa da DRAM do ESP32.
extern RegistroLog registrosLog[LIMITE_LOGS];
extern int quantidadeLogs;

// Total de eventos gerados desde o boot (o historico permanente fica no
// banco SQLite do servidor, nao no ESP32).
extern int quantidadeLogsHistorico;

void iniciarLogs();
void registrarLog(const char *evento, int indiceTrabalhador);
void apagarTodosLogs();
// Sobem a tarefa de envio (POST para o servidor) e o Wi-Fi.
void iniciarServidorWeb();
void processarServidorWeb();
void atualizarServidorWeb();

void carregarConfiguracaoWiFi();
void salvarConfiguracaoWiFi(const char *ssid, const char *senha);
void conectarWiFi();
const char *obterWiFiSSID();
const char *obterWiFiSenha();
bool wifiEstaConectado();