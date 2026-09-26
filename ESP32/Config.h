#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <stddef.h>

// Dimensoes do display 
constexpr int LARGURA_TELA = 320;
constexpr int ALTURA_TELA = 240;

// Comunicacao serial
constexpr uint32_t BAUD_SERIAL = 115200;

// Controle do rele
constexpr int PIN_RELE = 3;
constexpr uint8_t RELE_LIGADO = HIGH;
constexpr uint8_t RELE_DESLIGADO = LOW;

// Pinos do display ST7789
constexpr int PIN_DISPLAY_BACKLIGHT = 27;
constexpr int PIN_DISPLAY_DC = 2;
constexpr int PIN_DISPLAY_CS = 15;
constexpr int PIN_DISPLAY_SCK = 14;
constexpr int PIN_DISPLAY_MOSI = 13;

// Pinos do touch GT911
constexpr int PIN_TOUCH_SDA = 33;
constexpr int PIN_TOUCH_SCL = 32;
constexpr int PIN_TOUCH_RESET = 25;

// Pinos do PN532 em I2C
constexpr int PIN_PN532_SDA = 21;
constexpr int PIN_PN532_SCL = 22;
constexpr uint32_t VELOCIDADE_I2C_PN532 = 100000;

// Dimensoes da interface
constexpr int ALTURA_BARRA_NAVEGACAO = 46;
constexpr int ALTURA_CABECALHO = 28;

// Paleta visual
#define COR_FUNDO                  lv_color_hex(0xFFFFFF)
#define COR_SUPERFICIE             lv_color_hex(0xF1F5F9)
#define COR_SUPERFICIE_ALTERNATIVA lv_color_hex(0xE2E8F0)
#define COR_PRIMARIA               lv_color_hex(0x0E7490)
#define COR_PRIMARIA_ESCURO       lv_color_hex(0x155E75)
#define COR_SUCESSO                lv_color_hex(0x15803D)
#define COR_SUCESSO_FUNDO          lv_color_hex(0xDCFCE7)
#define COR_PERIGO                 lv_color_hex(0xDC2626)
#define COR_PERIGO_FUNDO           lv_color_hex(0xFEE2E2)
#define COR_ALERTA                 lv_color_hex(0xB45309)
#define COR_TEXTO_PRINCIPAL        lv_color_hex(0x0F172A)
#define COR_TEXTO_SECUNDARIO       lv_color_hex(0x64748B)
#define COR_TEXTO_SOBRE_DESTAQUE   lv_color_hex(0xFFFFFF)


// Nome do ESP32 na rede Wi-Fi e nome do equipamento monitorado.
constexpr const char *NOME_MDNS = "eloto";
constexpr const char *NOME_EQUIPAMENTO = "MESA PLANA";

// ------------------------------------------------------------
// SERVIDOR (backend com SQLite)
// ------------------------------------------------------------
// O ESP32 nao e mais servidor: ele MANDA (POST) os eventos de
// entrada/saida e o estado para o backend, que grava no SQLite. O
// site le desse banco.
//
// SERVIDOR_URL: endereco do computador que roda o backend (pasta
//   eloto-backend), na mesma rede Wi-Fi do ESP32. Use IP FIXO nesse
//   computador (reserva de DHCP no roteador), senao o IP muda e o ESP
//   deixa de achar o servidor. Sem barra no final.
// SERVIDOR_CHAVE: tem que ser igual a variavel ELOTO_KEY do backend.
constexpr const char *SERVIDOR_URL = "http://192.168.1.50:3000";
constexpr const char *SERVIDOR_CHAVE = "TCC";

// Estado (bloqueado/liberado + diagnostico) e reenviado a cada X ms
// mesmo sem mudanca; o site usa isso para saber se o ESP esta vivo.
constexpr uint32_t INTERVALO_HEARTBEAT_MS = 30000;

// Eventos que ainda nao chegaram ao servidor (servidor desligado, Wi-Fi
// fora) ficam nesta fila em RAM e sao reenviados. Se encher, descarta
// o mais antigo. Alocada em runtime (heap), nao conta na DRAM estatica.
constexpr int LIMITE_FILA_ENVIO = 16;

// IMPORTANTE - DRAM do ESP32 aqui esta no limite (o build estourou
// 14KB so de tentarmos subir esse numero para 100). Este array e
// ESTATICO: cada posicao a mais custa RAM o tempo todo, mesmo vazia.
//
// Por isso ele volta a ser pequeno e agora so alimenta o RAM buffer
// usado na tela de historico do proprio equipamento (ela so mostra
// os ultimos 8 registros mesmo, ver Telas.cpp). O historico completo
// fica no banco SQLite do backend, para onde o ESP32 manda cada
// evento com POST - ver Logs.cpp.
// Nao aumente este numero sem checar antes quanto de DRAM sobra
// no seu build (log de compilacao mostra "region overflowed").
constexpr int LIMITE_LOGS = 10;

// Cadastro de trabalhadores - tambem estatico, mesmo motivo acima.
constexpr int LIMITE_TRABALHADORES = 4;

struct Trabalhador {
    char nome[26];
    char funcao[20];
    char uidCartao[24];
    char empresa[26];
    bool ativo;
    bool cadastrado;
};

// Telas do sistema
// (a tela de diagnostico local foi removida - as informacoes de
// diagnostico agora vao no POST /api/estado, para o dashboard web,
// evitando gastar RAM/flash do ESP32 com uma tela extra)
enum Tela {
    TELA_INICIAL,
    TELA_TRABALHADORES,
    TELA_BLOQUEIO,
    TELA_MAIS,
    TELA_HISTORICO,
    TELA_WIFI
};