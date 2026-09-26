#include "UI.h"
#include "Hardware.h"
#include "Estado.h"

void limparTela()
{
    lv_obj_t *tela =
        lv_screen_active();


    lv_obj_clean(
        tela
    );


    lv_obj_set_style_bg_color(
        tela,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        tela,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_all(
        tela,
        0,
        LV_PART_MAIN
    );
}

lv_obj_t *criarPainel(
    lv_obj_t *parent,
    int x,
    int y,
    int w,
    int h,
    lv_color_t corBorda
)
{
    lv_obj_t *painel =
        lv_obj_create(parent);


    lv_obj_set_size(
        painel,
        w,
        h
    );


    lv_obj_set_pos(
        painel,
        x,
        y
    );


    lv_obj_set_style_bg_color(
        painel,
        COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        painel,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        painel,
        14,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        painel,
        1,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        painel,
        corBorda,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_opa(
        painel,
        LV_OPA_70,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_all(
        painel,
        8,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        painel,
        LV_OBJ_FLAG_SCROLLABLE
    );


    return painel;
}


lv_obj_t *criarCadeado(
    lv_obj_t *parent,
    int size,
    bool bloqueado,
    lv_event_cb_t evento
)
{
    lv_color_t cor =
        bloqueado
            ? COR_PERIGO
            : COR_SUCESSO;


    lv_obj_t *cont =
        lv_obj_create(parent);


    lv_obj_remove_style_all(
        cont
    );


    lv_obj_set_size(
        cont,
        size,
        size
    );


    lv_obj_remove_flag(
        cont,
        LV_OBJ_FLAG_SCROLLABLE
    );


    if (evento != NULL)
    {
        lv_obj_add_flag(
            cont,
            LV_OBJ_FLAG_CLICKABLE
        );


        lv_obj_add_event_cb(
            cont,
            evento,
            LV_EVENT_CLICKED,
            NULL
        );
    }


    int shackleW =
        size * 48 / 100;


    int shackleH =
        size * 48 / 100;


    int border =
        size * 11 / 100;


    if (border < 3)
        border = 3;


    lv_obj_t *shackle =
        lv_obj_create(cont);


    lv_obj_remove_style_all(
        shackle
    );


    lv_obj_set_size(
        shackle,
        shackleW,
        shackleH
    );


    lv_obj_align(
        shackle,
        LV_ALIGN_TOP_MID,
        0,
        2
    );


    lv_obj_set_style_bg_opa(
        shackle,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        shackle,
        border,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        shackle,
        cor,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_opa(
        shackle,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        shackle,
        shackleW / 2,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        shackle,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        shackle,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_t *corte =
        lv_obj_create(cont);


    lv_obj_remove_style_all(
        corte
    );


    lv_obj_set_size(
        corte,
        shackleW + border * 2,
        shackleH / 2
    );


    lv_obj_align(
        corte,
        LV_ALIGN_TOP_MID,
        0,
        shackleH / 2 + 2
    );


    lv_obj_set_style_bg_color(
        corte,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        corte,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        corte,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        corte,
        LV_OBJ_FLAG_SCROLLABLE
    );


    int bodyW =
        size * 82 / 100;


    int bodyH =
        size * 50 / 100;


    lv_obj_t *body =
        lv_obj_create(cont);


    lv_obj_remove_style_all(
        body
    );


    lv_obj_set_size(
        body,
        bodyW,
        bodyH
    );


    lv_obj_align(
        body,
        LV_ALIGN_BOTTOM_MID,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        body,
        cor,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        body,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        body,
        size * 10 / 100,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        body,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        body,
        LV_OBJ_FLAG_SCROLLABLE
    );


    int keySize =
        size * 18 / 100;


    if (keySize < 5)
        keySize = 5;


    lv_obj_t *key =
        lv_obj_create(body);


    lv_obj_remove_style_all(
        key
    );


    lv_obj_set_size(
        key,
        keySize,
        keySize
    );


    lv_obj_center(
        key
    );


    lv_obj_set_style_bg_color(
        key,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        key,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        key,
        LV_RADIUS_CIRCLE,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        key,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        key,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_t *keyStem =
        lv_obj_create(body);


    lv_obj_remove_style_all(
        keyStem
    );


    lv_obj_set_size(
        keyStem,
        keySize / 3,
        keySize
    );


    lv_obj_align(
        keyStem,
        LV_ALIGN_CENTER,
        0,
        keySize / 2
    );


    lv_obj_set_style_bg_color(
        keyStem,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        keyStem,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        keyStem,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        keyStem,
        LV_OBJ_FLAG_SCROLLABLE
    );


    return cont;
}

lv_obj_t *criarBotaoIcone(
    lv_obj_t *parent,
    const char *icone,
    const char *texto,
    int x,
    int y,
    int w,
    int h,
    lv_color_t cor,
    lv_event_cb_t evento
)
{
    lv_obj_t *btn =
        lv_button_create(parent);


    lv_obj_set_size(
        btn,
        w,
        h
    );


    lv_obj_set_pos(
        btn,
        x,
        y
    );


    lv_obj_set_style_bg_color(
        btn,
        cor,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        btn,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        btn,
        h / 2 < 16
            ? h / 2
            : 14,
        LV_PART_MAIN
    );


    lv_obj_set_style_shadow_width(
        btn,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        btn,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_color(
        btn,
        lv_color_darken(cor, 40),
        LV_STATE_PRESSED
    );


    lv_obj_set_style_translate_y(
        btn,
        1,
        LV_STATE_PRESSED
    );


    lv_obj_t *linha =
        lv_obj_create(btn);


    lv_obj_remove_style_all(
        linha
    );


    lv_obj_set_size(
        linha,
        LV_SIZE_CONTENT,
        LV_SIZE_CONTENT
    );


    lv_obj_set_flex_flow(
        linha,
        LV_FLEX_FLOW_ROW
    );


    lv_obj_set_flex_align(
        linha,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );


    lv_obj_set_style_pad_column(
        linha,
        6,
        LV_PART_MAIN
    );


    lv_obj_center(
        linha
    );


    lv_obj_remove_flag(
        linha,
        LV_OBJ_FLAG_CLICKABLE
    );


    if (icone != NULL)
    {
        lv_obj_t *lblIcone =
            lv_label_create(linha);


        lv_label_set_text(
            lblIcone,
            icone
        );


        lv_obj_set_style_text_color(
            lblIcone,
            COR_TEXTO_SOBRE_DESTAQUE,
            LV_PART_MAIN
        );


        lv_obj_set_style_text_font(
            lblIcone,
            &lv_font_montserrat_14,
            LV_PART_MAIN
        );


        lv_obj_remove_flag(
            lblIcone,
            LV_OBJ_FLAG_CLICKABLE
        );
    }


    if (
        texto != NULL &&
        texto[0] != '\0'
    )
    {
        lv_obj_t *label =
            lv_label_create(linha);


        lv_label_set_text(
            label,
            texto
        );


        lv_obj_set_style_text_color(
            label,
            COR_TEXTO_SOBRE_DESTAQUE,
            LV_PART_MAIN
        );


        lv_obj_set_style_text_font(
            label,
            &lv_font_montserrat_14,
            LV_PART_MAIN
        );


        lv_obj_remove_flag(
            label,
            LV_OBJ_FLAG_CLICKABLE
        );
    }


    if (evento != NULL)
    {
        lv_obj_add_event_cb(
            btn,
            evento,
            LV_EVENT_CLICKED,
            NULL
        );
    }


    return btn;
}


lv_obj_t *criarBotao(
    lv_obj_t *parent,
    const char *texto,
    int x,
    int y,
    int w,
    int h,
    lv_color_t cor,
    lv_event_cb_t evento
)
{
    return criarBotaoIcone(
        parent,
        NULL,
        texto,
        x,
        y,
        w,
        h,
        cor,
        evento
    );
}


lv_obj_t *criarTopo(
    lv_obj_t *tela,
    const char *titulo,
    bool comVoltar,
    lv_event_cb_t voltarCb
)
{
    lv_obj_t *topo =
        lv_obj_create(tela);


    lv_obj_set_size(
        topo,
        LARGURA_TELA,
        ALTURA_CABECALHO
    );


    lv_obj_set_pos(
        topo,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        topo,
        COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        topo,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        topo,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_all(
        topo,
        0,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        topo,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_set_style_border_width(
        topo,
        2,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        topo,
        COR_PRIMARIA,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_opa(
        topo,
        LV_OPA_70,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_side(
        topo,
        LV_BORDER_SIDE_BOTTOM,
        LV_PART_MAIN
    );


    int offsetX = 12;


    if (comVoltar)
    {
        lv_obj_t *btnVoltar =
            lv_button_create(topo);


        lv_obj_set_size(
            btnVoltar,
            26,
            22
        );


        lv_obj_align(
            btnVoltar,
            LV_ALIGN_LEFT_MID,
            6,
            0
        );


        lv_obj_set_style_bg_color(
            btnVoltar,
            COR_SUPERFICIE_ALTERNATIVA,
            LV_PART_MAIN
        );


        lv_obj_set_style_bg_opa(
            btnVoltar,
            LV_OPA_COVER,
            LV_PART_MAIN
        );


        lv_obj_set_style_radius(
            btnVoltar,
            6,
            LV_PART_MAIN
        );


        lv_obj_set_style_shadow_width(
            btnVoltar,
            0,
            LV_PART_MAIN
        );


        lv_obj_set_style_border_width(
            btnVoltar,
            0,
            LV_PART_MAIN
        );


        lv_obj_add_event_cb(
            btnVoltar,
            voltarCb,
            LV_EVENT_CLICKED,
            NULL
        );


        lv_obj_t *seta =
            lv_label_create(
                btnVoltar
            );


        lv_label_set_text(
            seta,
            "<"
        );


        lv_obj_set_style_text_color(
            seta,
            COR_PRIMARIA,
            LV_PART_MAIN
        );


        lv_obj_center(
            seta
        );


        lv_obj_remove_flag(
            seta,
            LV_OBJ_FLAG_CLICKABLE
        );


        offsetX = 38;
    }


    lv_obj_t *lbl =
        lv_label_create(topo);


    lv_label_set_text(
        lbl,
        titulo
    );


    lv_obj_set_style_text_color(
        lbl,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lbl,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        lbl,
        LV_ALIGN_LEFT_MID,
        offsetX,
        0
    );


    lv_obj_remove_flag(
        lbl,
        LV_OBJ_FLAG_CLICKABLE
    );


    return topo;
}

