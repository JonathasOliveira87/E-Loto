#pragma once
#include <lvgl.h>

void limparTela();
lv_obj_t *criarPainel(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t corBorda);
lv_obj_t *criarBotaoIcone(lv_obj_t *parent, const char *icone, const char *texto,
                          int x, int y, int w, int h, lv_color_t cor,
                          lv_event_cb_t evento);
lv_obj_t *criarBotao(lv_obj_t *parent, const char *texto,
                     int x, int y, int w, int h, lv_color_t cor,
                     lv_event_cb_t evento);
lv_obj_t *criarCadeado(lv_obj_t *parent, int tamanho, bool bloqueado,
                       lv_event_cb_t evento);
lv_obj_t *criarTopo(lv_obj_t *tela, const char *titulo,
                    bool comVoltar, lv_event_cb_t voltarCb);
