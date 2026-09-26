#include "Logs.h"
#include "Estado.h"
#include "RFID.h"
#include "Hardware.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/*
   MODULO DE LOGS / REDE

   O ESP32 NAO e mais servidor HTTP. Ele e cliente:
     - a cada ENTRADA/SAIDA de trabalhador, coloca o evento numa fila
       em RAM;
     - uma tarefa separada (FreeRTOS, nao trava tela nem RFID) manda a
       fila para o backend com POST /api/eventos, e reenvia se falhar;
     - a mesma tarefa manda o estado (POST /api/estado) quando o
       equipamento bloqueia/libera, quando muda o numero de
       trabalhadores ativos e a cada INTERVALO_HEARTBEAT_MS.

   Historico permanente = banco SQLite do backend. Nada de log na
   flash do ESP32 (so a configuracao do Wi-Fi).
*/

RegistroLog registrosLog[LIMITE_LOGS] = {};
int quantidadeLogs = 0;
int quantidadeLogsHistorico = 0;

static uint32_t proximoIdLog = 1;

static bool wifiTentado = false;
static bool wifiFalhou = false;

static uint32_t ultimoInicioWifi = 0;

// Controle de queda/reconexao do Wi-Fi (ver atualizarServidorWeb)
static bool eraConectado = false;
static bool jaConectouAntes = false;
static bool ntpConfigurado = false;
static uint32_t instanteQueda = 0;
static uint32_t ultimaTentativaReconexao = 0;
static uint32_t contadorReconexoes = 0;
static uint32_t ultimoDiagnosticoSerial = 0;

static const uint32_t INTERVALO_RECONEXAO_WIFI = 15000;
static const uint32_t TEMPO_RESET_RADIO_WIFI = 90000;
static const uint32_t INTERVALO_DIAGNOSTICO_MS = 10000;

static const char *ARQUIVO_WIFI = "/wifi.cfg";

// Versoes anteriores gravavam o historico aqui. Removido no boot para
// liberar a flash.
static const char *ARQUIVO_LOGS_ANTIGO = "/logs.json";

static char wifiSSID[33] = "";
static char wifiSenha[65] = "";


/* =========================================================
   DATA E HORA (so para a tela de historico local)
   ========================================================= */

static void obterDataHora(char *data, size_t tamData,
                          char *hora, size_t tamHora)
{
    time_t agora = time(nullptr);

    if (agora < 100000)
    {
        strncpy(data, "SEM DATA", tamData - 1);
        data[tamData - 1] = '\0';

        strncpy(hora, "--:--:--", tamHora - 1);
        hora[tamHora - 1] = '\0';

        return;
    }

    struct tm info;

    localtime_r(&agora, &info);

    strftime(data, tamData, "%d/%m/%Y", &info);
    strftime(hora, tamHora, "%H:%M:%S", &info);
}


/* =========================================================
   CONFIGURAÇÃO WI-FI
   ========================================================= */

void carregarConfiguracaoWiFi()
{
    wifiSSID[0] = '\0';
    wifiSenha[0] = '\0';

    File f = LittleFS.open(ARQUIVO_WIFI, FILE_READ);

    if (!f)
        return;

    String linha1 = f.readStringUntil('\n');
    String linha2 = f.readStringUntil('\n');

    f.close();

    linha1.trim();
    linha2.trim();

    strncpy(wifiSSID, linha1.c_str(), sizeof(wifiSSID) - 1);
    wifiSSID[sizeof(wifiSSID) - 1] = '\0';

    strncpy(wifiSenha, linha2.c_str(), sizeof(wifiSenha) - 1);
    wifiSenha[sizeof(wifiSenha) - 1] = '\0';
}


void salvarConfiguracaoWiFi(const char *ssid, const char *senha)
{
    strncpy(wifiSSID,
            ssid ? ssid : "",
            sizeof(wifiSSID) - 1);

    wifiSSID[sizeof(wifiSSID) - 1] = '\0';

    strncpy(wifiSenha,
            senha ? senha : "",
            sizeof(wifiSenha) - 1);

    wifiSenha[sizeof(wifiSenha) - 1] = '\0';

    File f = LittleFS.open(ARQUIVO_WIFI, FILE_WRITE);

    if (!f)
        return;

    f.println(wifiSSID);
    f.println(wifiSenha);

    f.close();
}


const char *obterWiFiSSID()
{
    return wifiSSID;
}


const char *obterWiFiSenha()
{
    return wifiSenha;
}


/* =========================================================
   STATUS WI-FI
   ========================================================= */

bool wifiEstaConectado()
{
    return WiFi.status() == WL_CONNECTED;
}


/* =========================================================
   CONECTAR WI-FI
   ========================================================= */

static bool eventosWiFiRegistrados = false;

// Mostra no Serial Monitor o MOTIVO de cada queda do Wi-Fi e quando
// o IP volta (ex.: 200 = beacon timeout, 201 = roteador nao
// encontrado, 8 = roteador derrubou o ESP, 15 = falha de handshake).
static void aoEventoWiFi(arduino_event_id_t evento, arduino_event_info_t info)
{
    if (evento == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        Serial.printf(
            "[WIFI] desconectado, motivo=%d\n",
            (int)info.wifi_sta_disconnected.reason
        );
    }
    else if (evento == ARDUINO_EVENT_WIFI_STA_GOT_IP)
    {
        Serial.printf(
            "[WIFI] conectado, IP=%s RSSI=%d\n",
            WiFi.localIP().toString().c_str(),
            (int)WiFi.RSSI()
        );
    }
}


void conectarWiFi()
{
    if (wifiSSID[0] == '\0')
        return;

    if (!eventosWiFiRegistrados)
    {
        WiFi.onEvent(aoEventoWiFi);
        eventosWiFiRegistrados = true;
    }

    WiFi.persistent(false);

    WiFi.mode(WIFI_STA);

    WiFi.setHostname(NOME_MDNS);

    WiFi.setAutoReconnect(true);

    WiFi.disconnect(false);

    delay(30);

    WiFi.begin(wifiSSID, wifiSenha);

    // Sem economia de energia do radio: no modo padrao o ESP32
    // "cochila" entre beacons do roteador e pode perder pacotes.
    WiFi.setSleep(false);

    wifiTentado = true;
    wifiFalhou = false;

    ultimoInicioWifi = millis();
    instanteQueda = ultimoInicioWifi;
    ultimaTentativaReconexao = ultimoInicioWifi;
}


// Ultimo recurso quando o Wi-Fi fica muito tempo fora: desliga e liga
// o radio. NAO reinicia o ESP - reiniciar zeraria a equipe ativa e
// liberaria o equipamento bloqueado.
static void resetarRadioWiFi()
{
    Serial.println("[WIFI] resetando o radio");

    WiFi.disconnect(true, false);
    delay(100);

    WiFi.mode(WIFI_OFF);
    delay(100);

    conectarWiFi();
}


/* =========================================================
   FILA DE ENVIO PARA O SERVIDOR
   ========================================================= */

struct EventoPendente
{
    char origemId[20];   // "<idBoot>-<seq>": o servidor ignora repetidos
    char evento[12];
    char nome[26];
    char funcao[20];
    char uidCartao[24];
    uint32_t ts;         // epoch (NTP); 0 = ainda sem hora, o servidor usa a dele
};

static EventoPendente *fila = nullptr;
static int filaInicio = 0;
static int filaTotal = 0;
static SemaphoreHandle_t filaMutex = nullptr;

static uint32_t idBoot = 0;
static uint32_t sequenciaEnvio = 0;

static bool tarefaEnvioCriada = false;


static void enfileirarEvento(const RegistroLog &l)
{
    if (fila == nullptr || filaMutex == nullptr)
        return;

    if (xSemaphoreTake(filaMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return;

    if (filaTotal >= LIMITE_FILA_ENVIO)
    {
        // Fila cheia (servidor fora por muito tempo): descarta o mais antigo.
        filaInicio = (filaInicio + 1) % LIMITE_FILA_ENVIO;
        filaTotal--;
    }

    int pos = (filaInicio + filaTotal) % LIMITE_FILA_ENVIO;

    EventoPendente &e = fila[pos];

    snprintf(
        e.origemId,
        sizeof(e.origemId),
        "%08lx-%lu",
        (unsigned long)idBoot,
        (unsigned long)(++sequenciaEnvio)
    );

    strncpy(e.evento, l.evento, sizeof(e.evento) - 1);
    e.evento[sizeof(e.evento) - 1] = '\0';

    strncpy(e.nome, l.nome, sizeof(e.nome) - 1);
    e.nome[sizeof(e.nome) - 1] = '\0';

    strncpy(e.funcao, l.funcao, sizeof(e.funcao) - 1);
    e.funcao[sizeof(e.funcao) - 1] = '\0';

    strncpy(e.uidCartao, l.uidCartao, sizeof(e.uidCartao) - 1);
    e.uidCartao[sizeof(e.uidCartao) - 1] = '\0';

    time_t agora = time(nullptr);
    e.ts = agora > 100000 ? (uint32_t)agora : 0;

    filaTotal++;

    xSemaphoreGive(filaMutex);
}


static bool copiarPrimeiroEvento(EventoPendente &destino)
{
    bool ok = false;

    if (xSemaphoreTake(filaMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        if (filaTotal > 0)
        {
            destino = fila[filaInicio];
            ok = true;
        }

        xSemaphoreGive(filaMutex);
    }

    return ok;
}


static void removerPrimeiroEvento(const char *origemId)
{
    if (xSemaphoreTake(filaMutex, pdMS_TO_TICKS(50)) == pdTRUE)
    {
        // Confere o id: se a fila estourou enquanto enviava, o primeiro
        // ja pode ser outro.
        if (filaTotal > 0 &&
            strcmp(fila[filaInicio].origemId, origemId) == 0)
        {
            filaInicio = (filaInicio + 1) % LIMITE_FILA_ENVIO;
            filaTotal--;
        }

        xSemaphoreGive(filaMutex);
    }
}


/* =========================================================
   HTTP (rodando na tarefa de envio)
   ========================================================= */

// Copia src para dst escapando aspas e barra e descartando caracteres
// de controle, para o JSON nunca ficar quebrado.
static void escaparJson(char *dst, size_t tam, const char *src)
{
    size_t o = 0;

    for (; *src && o + 2 < tam; src++)
    {
        unsigned char c = (unsigned char)*src;

        if (c == '"' || c == '\\')
        {
            dst[o++] = '\\';
            dst[o++] = (char)c;
        }
        else if (c < 0x20)
        {
            continue;
        }
        else
        {
            dst[o++] = (char)c;
        }
    }

    dst[o] = '\0';
}


// Retorna o codigo HTTP (200..) ou negativo se nem conectou.
static int postarJson(const char *caminho, const char *corpo)
{
    char url[128];

    snprintf(url, sizeof(url), "%s%s", SERVIDOR_URL, caminho);

    WiFiClient cliente;
    HTTPClient http;

    // Timeouts curtos: se o servidor estiver desligado a tarefa de
    // envio espera pouco e tenta de novo depois.
    http.setConnectTimeout(2000);
    http.setTimeout(3000);
    http.setReuse(false);

    if (!http.begin(cliente, url))
        return -1;

    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", SERVIDOR_CHAVE);

    int codigo = http.POST((uint8_t *)corpo, strlen(corpo));

    http.end();

    return codigo;
}


static bool postarEvento(const EventoPendente &e)
{
    char equipamento[64];
    char evento[28];
    char nome[56];
    char funcao[44];
    char uid[52];

    escaparJson(equipamento, sizeof(equipamento), NOME_EQUIPAMENTO);
    escaparJson(evento, sizeof(evento), e.evento);
    escaparJson(nome, sizeof(nome), e.nome);
    escaparJson(funcao, sizeof(funcao), e.funcao);
    escaparJson(uid, sizeof(uid), e.uidCartao);

    char corpo[440];

    snprintf(
        corpo,
        sizeof(corpo),
        "{\"origem_id\":\"%s\","
        "\"equipamento\":\"%s\","
        "\"evento\":\"%s\","
        "\"trabalhador\":\"%s\","
        "\"funcao\":\"%s\","
        "\"uid\":\"%s\","
        "\"ts\":%lu}",
        e.origemId,
        equipamento,
        evento,
        nome,
        funcao,
        uid,
        (unsigned long)e.ts
    );

    int codigo = postarJson("/api/eventos", corpo);

    Serial.printf(
        "[ENVIO] evento %s %s -> HTTP %d\n",
        e.evento,
        e.origemId,
        codigo
    );

    if (codigo >= 200 && codigo < 300)
        return true;

    // 4xx = o servidor recusou este evento (chave errada, dado invalido).
    // Reenviar nao adianta e travaria a fila: descarta.
    if (codigo >= 400 && codigo < 500)
    {
        Serial.println("[ENVIO] servidor recusou o evento, descartado");
        return true;
    }

    // Sem resposta / 5xx: mantem na fila e tenta de novo depois.
    return false;
}


static bool postarEstado()
{
    char equipamento[64];

    escaparJson(equipamento, sizeof(equipamento), NOME_EQUIPAMENTO);

    // Ultimas leituras (bancada), formatadas como array JSON "[12,34,...]".
    // So para facilitar a tabela do TCC - o servidor nao grava isso no
    // banco, so repassa para quem estiver com o painel aberto.
    uint16_t ultimasLeituras[30];
    size_t qtdUltimas = copiarUltimasLeiturasRFID(ultimasLeituras, 30);

    char listaLeituras[220] = "[";
    size_t posLista = 1;

    for (size_t i = 0; i < qtdUltimas && posLista < sizeof(listaLeituras) - 8; i++)
    {
        int escritos = snprintf(
            listaLeituras + posLista,
            sizeof(listaLeituras) - posLista,
            "%s%u",
            i > 0 ? "," : "",
            (unsigned)ultimasLeituras[i]
        );

        if (escritos < 0)
            break;

        posLista += (size_t)escritos;
    }

    if (posLista < sizeof(listaLeituras) - 1)
    {
        listaLeituras[posLista++] = ']';
        listaLeituras[posLista] = '\0';
    }
    else
    {
        // nao deveria acontecer (30 leituras de ate 5 digitos cabem
        // tranquilo no buffer), mas fecha o array por seguranca.
        strcpy(listaLeituras + sizeof(listaLeituras) - 3, "]");
    }

    char corpo[820];

    snprintf(
        corpo,
        sizeof(corpo),
        "{\"equipamento\":\"%s\","
        "\"status\":\"%s\","
        "\"trabalhadores_ativos\":%d,"
        "\"touchscreen\":%s,"
        "\"rfid_disponivel\":%s,"
        "\"rfid_leituras_medidas\":%lu,"
        "\"rfid_tempo_resposta_ultimo_ms\":%lu,"
        "\"rfid_tempo_resposta_medio_ms\":%.1f,"
        "\"rfid_tempo_resposta_desvio_ms\":%.1f,"
        "\"rfid_ultimas_leituras_ms\":%s,"
        "\"wifi_rssi\":%d,"
        "\"heap_livre\":%lu,"
        "\"heap_min\":%lu,"
        "\"uptime_s\":%lu,"
        "\"wifi_reconexoes\":%lu}",
        equipamento,
        equipamentoBloqueado ? "BLOQUEADO" : "LIBERADO",
        quantidadeTrabalhadoresAtivos,
        touchDisponivel ? "true" : "false",
        pn532Disponivel ? "true" : "false",
        (unsigned long)rfidQuantidadeLeituras,
        (unsigned long)rfidUltimoTempoRespostaMs,
        rfidTempoRespostaMedioMs,
        rfidTempoRespostaDesvioMs,
        listaLeituras,
        (int)WiFi.RSSI(),
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)ESP.getMinFreeHeap(),
        (unsigned long)(millis() / 1000),
        (unsigned long)contadorReconexoes
    );

    int codigo = postarJson("/api/estado", corpo);

    return codigo >= 200 && codigo < 300;
}


static inline bool passou(uint32_t alvo)
{
    return (int32_t)(millis() - alvo) >= 0;
}


/* =========================================================
   TAREFA DE ENVIO
   ========================================================= */
//
// Roda no core 0 (o loop() do Arduino roda no core 1), entao um
// servidor lento ou desligado nunca trava a tela nem o leitor RFID.

static void tarefaEnvio(void *param)
{
    (void)param;

    uint32_t proximoHeartbeat = 0;
    uint32_t proximaTentativaEstado = 0;

    int bloqueioEnviado = -1;
    int ativosEnviado = -1;

    for (;;)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // 1) Eventos pendentes, do mais antigo para o mais novo.
        EventoPendente ev;

        if (copiarPrimeiroEvento(ev))
        {
            if (postarEvento(ev))
            {
                removerPrimeiroEvento(ev.origemId);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(3000));
            }

            continue;
        }

        // 2) Estado: quando mudou ou no heartbeat.
        int bloqueioAtual = equipamentoBloqueado ? 1 : 0;
        int ativosAtual = quantidadeTrabalhadoresAtivos;

        bool mudou =
            bloqueioAtual != bloqueioEnviado ||
            ativosAtual != ativosEnviado;

        if (passou(proximaTentativaEstado) &&
            (mudou || passou(proximoHeartbeat)))
        {
            if (postarEstado())
            {
                bloqueioEnviado = bloqueioAtual;
                ativosEnviado = ativosAtual;

                proximoHeartbeat = millis() + INTERVALO_HEARTBEAT_MS;
                proximaTentativaEstado = millis();
            }
            else
            {
                proximaTentativaEstado = millis() + 5000;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


/* =========================================================
   INICIALIZAÇÃO DOS LOGS
   ========================================================= */

void iniciarLogs()
{
    if (!LittleFS.begin(true))
    {
        return;
    }

    carregarConfiguracaoWiFi();

    // Limpa o arquivo de historico das versoes antigas.
    LittleFS.remove(ARQUIVO_LOGS_ANTIGO);

    quantidadeLogsHistorico = 0;

    // Fila de envio no heap (nao na DRAM estatica).
    if (fila == nullptr)
    {
        fila = (EventoPendente *)calloc(
            LIMITE_FILA_ENVIO,
            sizeof(EventoPendente)
        );
    }

    if (filaMutex == nullptr)
    {
        filaMutex = xSemaphoreCreateMutex();
    }

    // Identifica este boot: junto com o numero sequencial forma o id
    // do evento, que o servidor usa para nao gravar duas vezes.
    idBoot = esp_random();
}


/* =========================================================
   REGISTRAR LOG
   ========================================================= */

void registrarLog(const char *evento, int indiceTrabalhador)
{
    if (indiceTrabalhador < 0 ||
        indiceTrabalhador >= quantidadeTrabalhadoresCadastrados)
    {
        return;
    }

    // ---- Buffer pequeno em RAM (so para a tela local do equipamento) ----

    if (quantidadeLogs >= LIMITE_LOGS)
    {
        for (int i = 1; i < LIMITE_LOGS; i++)
        {
            registrosLog[i - 1] = registrosLog[i];
        }

        quantidadeLogs = LIMITE_LOGS - 1;
    }

    RegistroLog &l = registrosLog[quantidadeLogs++];

    l.id = proximoIdLog++;


    strncpy(l.evento,
            evento,
            sizeof(l.evento) - 1);

    l.evento[sizeof(l.evento) - 1] = '\0';


    Trabalhador &t = cadastroTrabalhadores[indiceTrabalhador];


    strncpy(l.nome,
            t.nome,
            sizeof(l.nome) - 1);

    l.nome[sizeof(l.nome) - 1] = '\0';


    strncpy(l.funcao,
            t.funcao,
            sizeof(l.funcao) - 1);

    l.funcao[sizeof(l.funcao) - 1] = '\0';


    strncpy(l.uidCartao,
            t.uidCartao,
            sizeof(l.uidCartao) - 1);

    l.uidCartao[sizeof(l.uidCartao) - 1] = '\0';


    strncpy(l.equipamento,
            NOME_EQUIPAMENTO,
            sizeof(l.equipamento) - 1);

    l.equipamento[sizeof(l.equipamento) - 1] = '\0';


    obterDataHora(
        l.data,
        sizeof(l.data),
        l.hora,
        sizeof(l.hora)
    );


    // ---- Envio para o servidor (fila; nao bloqueia) ----

    enfileirarEvento(l);
    quantidadeLogsHistorico++;
}


/* =========================================================
   APAGAR LOGS DA TELA
   ========================================================= */
//
// Limpa so a lista da tela de historico do equipamento. O historico
// de verdade fica no banco do servidor e nao e apagado por aqui
// (registro de seguranca).

void apagarTodosLogs()
{
    quantidadeLogs = 0;
    quantidadeLogsHistorico = 0;

    proximoIdLog = 1;
}


/* =========================================================
   INICIAR (nome mantido por compatibilidade com o .ino)
   ========================================================= */

void iniciarServidorWeb()
{
    // A tarefa de envio sobe mesmo sem Wi-Fi configurado: se a rede for
    // cadastrada depois pela tela, ela ja esta rodando.
    if (!tarefaEnvioCriada && fila != nullptr && filaMutex != nullptr)
    {
        BaseType_t ok = xTaskCreatePinnedToCore(
            tarefaEnvio,
            "envio",
            8192,
            nullptr,
            1,
            nullptr,
            0
        );

        tarefaEnvioCriada = (ok == pdPASS);

        if (!tarefaEnvioCriada)
        {
            Serial.println("[ENVIO] nao foi possivel criar a tarefa de envio");
        }
    }

    if (wifiSSID[0] == '\0')
        return;

    conectarWiFi();
}


/* =========================================================
   MANUTENCAO DO WI-FI (nome mantido por compatibilidade)
   ========================================================= */

void atualizarServidorWeb()
{
    if (wifiSSID[0] == '\0')
        return;


    wl_status_t status = WiFi.status();
    uint32_t agora = millis();


    // Diagnostico no Serial Monitor a cada 10 s.
    if (agora - ultimoDiagnosticoSerial >= INTERVALO_DIAGNOSTICO_MS)
    {
        ultimoDiagnosticoSerial = agora;

        Serial.printf(
            "[DIAG] wifi=%d rssi=%d heap=%lu heapMin=%lu reconexoes=%lu fila=%d\n",
            (int)status,
            status == WL_CONNECTED ? (int)WiFi.RSSI() : 0,
            (unsigned long)ESP.getFreeHeap(),
            (unsigned long)ESP.getMinFreeHeap(),
            (unsigned long)contadorReconexoes,
            filaTotal
        );
    }


    /* -----------------------------------------------------
       WI-FI CONECTADO
       ----------------------------------------------------- */

    if (status == WL_CONNECTED)
    {
        if (!eraConectado)
        {
            // Acabou de (re)conectar.
            eraConectado = true;

            WiFi.setSleep(false);

            if (jaConectouAntes)
                contadorReconexoes++;

            jaConectouAntes = true;
            wifiFalhou = false;
        }

        if (!ntpConfigurado)
        {
            // Hora para carimbar os eventos (fuso de Brasilia).
            configTime(
                -3 * 3600,
                0,
                "pool.ntp.org",
                "time.nist.gov"
            );

            ntpConfigurado = true;
        }

        return;
    }


    /* -----------------------------------------------------
       WI-FI DESCONECTADO
       ----------------------------------------------------- */

    if (eraConectado)
    {
        // Acabou de cair.
        eraConectado = false;
        instanteQueda = agora;
        ultimaTentativaReconexao = agora;
    }

    if (wifiTentado &&
        agora - ultimaTentativaReconexao >=
            INTERVALO_RECONEXAO_WIFI)
    {
        ultimaTentativaReconexao = agora;
        wifiFalhou = true;

        if (agora - instanteQueda >= TEMPO_RESET_RADIO_WIFI)
        {
            // Mais de 90 s fora: reinicia o radio.
            resetarRadioWiFi();
        }
        else if (status != WL_IDLE_STATUS)
        {
            // Pede reconexao ao driver, SEM chamar WiFi.begin() de novo:
            // begin()/disconnect() no meio de uma tentativa em andamento
            // pode deixar o Wi-Fi travado ate reiniciar o ESP.
            WiFi.reconnect();
        }
    }
}


/* =========================================================
   PROCESSAR (chamado a cada volta do loop)
   ========================================================= */

void processarServidorWeb()
{
    atualizarServidorWeb();
}
