#pragma once
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "Touch_GT911.h"
#include "Config.h"

extern Arduino_DataBus *barramentoDisplay;
extern Arduino_GFX *displayGrafico;
extern Touch_GT911 controladorTouch;
extern TwoWire barramentoNfc;
extern Adafruit_PN532 leitorNfc;

extern bool pn532Disponivel;
extern bool touchDisponivel;
extern lv_color_t *bufferDesenho1;
extern lv_color_t *bufferDesenho2;
extern lv_display_t *displayLvgl;
extern lv_indev_t *entradaTouchLvgl;

uint32_t lvgl_tick();
void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixelMap);
void testarGT911I2C();
void atualizarRele();
