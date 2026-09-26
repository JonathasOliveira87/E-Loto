#include "Config.h"
#include "Hardware.h"
#include "Estado.h"
#include "RFID.h"
#include "Touch.h"
#include "Telas.h"
#include "Logs.h"

// ------------------------------------------------------------
// WATCHDOG DE TAREFA
// ------------------------------------------------------------
// Rede de seguranca: se o loop() por algum motivo travar de verdade
// (ex.: I2C do PN532 preso apos uma colisao de cartoes), o ESP32
// reinicia sozinho depois de WDT_TIMEOUT_S segundos, em vez de ficar
// travado ate alguem tirar da tomada.
//
// OBS: a API muda entre versoes do core ESP32 Arduino.
//  - Core 2.x (esp_task_wdt_init(uint32_t, bool)): use o bloco A.
//  - Core 3.x / IDF5 (esp_task_wdt_init(config)):  use o bloco B.
// Deixe so um dos dois blocos ativo (comente o outro) conforme a
// versao do core instalada na sua IDE/Arduino CLI. O log de
// compilacao mostra a versao (ex.: "esp32/hardware/esp32/3.3.11" ->
// isso e core 3.x, use o BLOCO B).

#include <esp_task_wdt.h>

constexpr uint32_t WDT_TIMEOUT_S = 10;

static void iniciarWatchdog()
{
    // ---- BLOCO A: core ESP32 Arduino 2.x ----
    // esp_task_wdt_init(WDT_TIMEOUT_S, true); // true = reinicia ao estourar
    // esp_task_wdt_add(NULL);                 // monitora a task atual (loop)

    // ---- BLOCO B: core ESP32 Arduino 3.x / IDF5 ----
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&wdtConfig);
    esp_task_wdt_add(NULL);
}

void setup()
{

    delay(1000);

    // Necessario para os prints de diagnostico (ex.: tempo de resposta
    // do RFID) aparecerem no Serial Monitor durante os testes de
    // bancada.
    Serial.begin(BAUD_SERIAL);

    // Inicializacao do rele
    pinMode(PIN_RELE, OUTPUT);
    digitalWrite(PIN_RELE, RELE_DESLIGADO);


    // Inicializacao do display
    pinMode(PIN_DISPLAY_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_DISPLAY_BACKLIGHT, HIGH);

    displayGrafico->begin(80000000);
    displayGrafico->fillScreen(0x0000);

    // Barramento I2C do GT911
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL);
    testarGT911I2C();

    // Historico persistente.
    // O Wi-Fi nao e iniciado aqui para nao bloquear/piscar a tela.
    iniciarLogs();

    // Inicializacao do RFID
    iniciarPN532();

    // Inicializacao do touch
    controladorTouch.begin(GT911_ADDR1);
    controladorTouch.setRotation(ROTATION_INVERTED);


    // Inicializacao do LVGL
    lv_init();
    lv_tick_set_cb(lvgl_tick);

    size_t quantidadePixelsBuffer = LARGURA_TELA * 40;

    bufferDesenho1 = (lv_color_t *)malloc(
        quantidadePixelsBuffer * sizeof(lv_color_t)
    );

    bufferDesenho2 = (lv_color_t *)malloc(
        quantidadePixelsBuffer * sizeof(lv_color_t)
    );

    if (bufferDesenho1 == nullptr || bufferDesenho2 == nullptr)
    {


        while (true)
        {
            delay(1000);
        }
    }

    displayLvgl = lv_display_create(LARGURA_TELA, ALTURA_TELA);

    lv_display_set_flush_cb(
        displayLvgl,
        my_disp_flush
    );

    lv_display_set_buffers(
        displayLvgl,
        bufferDesenho1,
        bufferDesenho2,
        quantidadePixelsBuffer * sizeof(lv_color_t),
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    // Entrada de toque do LVGL
    entradaTouchLvgl = lv_indev_create();

    lv_indev_set_type(
        entradaTouchLvgl,
        LV_INDEV_TYPE_POINTER
    );

    lv_indev_set_read_cb(
        entradaTouchLvgl,
        my_touch_read
    );

    // Estado inicial do sistema
    quantidadeTrabalhadoresAtivos = 0;
    quantidadeTrabalhadoresCadastrados = 0;
    equipamentoBloqueado = false;

    atualizarRele();

    // Primeira tela
    carregarTela(TELA_INICIAL);

    // Inicia o Wi-Fi somente depois que a interface ja esta pronta.
    // A funcao nao bloqueia esperando conexao.
    iniciarServidorWeb();

    ultimoToqueMillis = millis();

    // Watchdog por ultimo, depois que tudo o que pode demorar no boot
    // (LittleFS, PN532, LVGL) ja rodou.
    iniciarWatchdog();

    if (pn532Disponivel)
    {

    }
    else
    {

    }

}

void loop()
{

    // Alimenta o watchdog a cada volta do loop. Se o loop nao voltar
    // aqui dentro de WDT_TIMEOUT_S segundos, o ESP32 reinicia sozinho.
    esp_task_wdt_reset();

    // Servidor HTTP / JSON e conexao Wi-Fi
    processarServidorWeb();
    atualizarStatusWiFiTela();

    // Leitura do RFID
    lerRFID();

    // Processamento dos eventos do LVGL
    lv_timer_handler();

    delay(5);
}
