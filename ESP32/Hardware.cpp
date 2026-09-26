#include "Hardware.h"
#include "Estado.h"
#include <Arduino.h>

Arduino_DataBus *barramentoDisplay = new Arduino_ESP32SPI(
    PIN_DISPLAY_DC, PIN_DISPLAY_CS, PIN_DISPLAY_SCK,
    PIN_DISPLAY_MOSI, GFX_NOT_DEFINED
);

Arduino_GFX *displayGrafico = new Arduino_ST7789(
    barramentoDisplay, -1, 1, true
);

Touch_GT911 controladorTouch(
    PIN_TOUCH_SDA, PIN_TOUCH_SCL,
    PIN_TOUCH_RESET,
    LARGURA_TELA, ALTURA_TELA
);

TwoWire barramentoNfc = TwoWire(1);
Adafruit_PN532 leitorNfc(-1, -1, &barramentoNfc);

bool pn532Disponivel = false;
bool touchDisponivel = false;

lv_color_t *bufferDesenho1 = nullptr;
lv_color_t *bufferDesenho2 = nullptr;
lv_display_t *displayLvgl = nullptr;
lv_indev_t *entradaTouchLvgl = nullptr;

uint32_t lvgl_tick()
{
    return millis();
}

void my_disp_flush(
    lv_display_t *display,
    const lv_area_t *area,
    uint8_t *pixelMap
)
{
    uint32_t largura = area->x2 - area->x1 + 1;
    uint32_t altura = area->y2 - area->y1 + 1;

    displayGrafico->draw16bitRGBBitmap(
        area->x1, area->y1,
        (uint16_t *)pixelMap,
        largura, altura
    );

    lv_display_flush_ready(display);
}

// Verifica se o controlador de touch GT911 responde no barramento I2C,
// no endereco esperado (0x5D ou, em alguns modulos, 0x14). O resultado
// e guardado em touchDisponivel e exposto no /estado.json para o
// dashboard web - a tela de diagnostico local foi removida do ESP32
// para nao gastar RAM/flash com uma segunda copia dessa informacao.
void testarGT911I2C()
{
    Wire.beginTransmission(GT911_ADDR1);
    uint8_t erro1 = Wire.endTransmission();

    Wire.beginTransmission(GT911_ADDR2);
    uint8_t erro2 = Wire.endTransmission();

    touchDisponivel = (erro1 == 0) || (erro2 == 0);
}

void atualizarRele()
{
    digitalWrite(
        PIN_RELE,
        equipamentoBloqueado ? RELE_LIGADO : RELE_DESLIGADO
    );
}
