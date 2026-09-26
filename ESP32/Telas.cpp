#include "Telas.h"
#include "UI.h"
#include "Estado.h"
#include "Hardware.h"
#include "RFID.h"
#include "Touch.h"
#include "Logs.h"
#include <Arduino.h>
#include <string.h>
#include <WiFi.h>


// ============================================================
// NAVEGAÇÃO
// ============================================================

static void cb_ir_inicial(
    lv_event_t *e
)
{
    carregarTela(
        TELA_INICIAL
    );
}


static void cb_ir_trabalhadores(
    lv_event_t *e
)
{
    carregarTela(
        TELA_TRABALHADORES
    );
}


static void cb_ir_bloqueio(
    lv_event_t *e
)
{
    carregarTela(
        TELA_BLOQUEIO
    );
}


static void cb_ir_mais(
    lv_event_t *e
)
{
    carregarTela(
        TELA_MAIS
    );
}


static void cb_ir_historico(
    lv_event_t *e
)
{
    carregarTela(
        TELA_HISTORICO
    );
}


struct AbaNav
{
    const char *nome;
    lv_event_cb_t cb;
};


void criarNavBar(
    lv_obj_t *tela,
    int abaAtiva
)
{
    lv_obj_t *nav =
        lv_obj_create(tela);


    lv_obj_set_size(
        nav,
        LARGURA_TELA,
        ALTURA_BARRA_NAVEGACAO
    );


    lv_obj_align(
        nav,
        LV_ALIGN_BOTTOM_MID,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        nav,
        COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        nav,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        nav,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        nav,
        1,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        nav,
        COR_SUPERFICIE_ALTERNATIVA,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_side(
        nav,
        LV_BORDER_SIDE_TOP,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_all(
        nav,
        0,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        nav,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_remove_flag(
        nav,
        LV_OBJ_FLAG_CLICKABLE
    );


    AbaNav abas[4] =
    {
        {
            "INICIO",
            cb_ir_inicial
        },

        {
            "EQUIPE",
            cb_ir_trabalhadores
        },

        {
            "BLOQ.",
            cb_ir_bloqueio
        },

        {
            "MAIS",
            cb_ir_mais
        }
    };


    int larguraAba =
        LARGURA_TELA / 4;


    for (
        int i = 0;
        i < 4;
        i++
    )
    {
        lv_obj_t *aba =
            lv_obj_create(nav);


        lv_obj_set_size(
            aba,
            larguraAba,
            ALTURA_BARRA_NAVEGACAO - 2
        );


        lv_obj_set_pos(
            aba,
            i * larguraAba,
            1
        );


        lv_obj_set_style_radius(
            aba,
            0,
            LV_PART_MAIN
        );


        lv_obj_set_style_border_width(
            aba,
            0,
            LV_PART_MAIN
        );


        lv_obj_set_style_pad_all(
            aba,
            0,
            LV_PART_MAIN
        );


        lv_obj_remove_flag(
            aba,
            LV_OBJ_FLAG_SCROLLABLE
        );


        lv_obj_add_flag(
            aba,
            LV_OBJ_FLAG_CLICKABLE
        );


        lv_obj_add_event_cb(
            aba,
            abas[i].cb,
            LV_EVENT_CLICKED,
            NULL
        );


        bool ativa =
            (i == abaAtiva);


        lv_obj_set_style_bg_color(
            aba,
            ativa
                ? COR_SUPERFICIE_ALTERNATIVA
                : COR_SUPERFICIE,
            LV_PART_MAIN
        );


        lv_obj_set_style_bg_opa(
            aba,
            LV_OPA_COVER,
            LV_PART_MAIN
        );


        if (ativa)
        {
            lv_obj_t *tracoIndicador =
                lv_obj_create(aba);


            lv_obj_remove_style_all(
                tracoIndicador
            );


            lv_obj_set_size(
                tracoIndicador,
                26,
                3
            );


            lv_obj_align(
                tracoIndicador,
                LV_ALIGN_TOP_MID,
                0,
                0
            );


            lv_obj_set_style_radius(
                tracoIndicador,
                3,
                LV_PART_MAIN
            );


            lv_obj_set_style_bg_color(
                tracoIndicador,
                COR_PRIMARIA,
                LV_PART_MAIN
            );


            lv_obj_set_style_bg_opa(
                tracoIndicador,
                LV_OPA_COVER,
                LV_PART_MAIN
            );


            lv_obj_remove_flag(
                tracoIndicador,
                LV_OBJ_FLAG_CLICKABLE
            );
        }


        lv_obj_t *lbl =
            lv_label_create(aba);


        lv_label_set_text(
            lbl,
            abas[i].nome
        );


        lv_obj_set_style_text_color(
            lbl,
            ativa
                ? COR_PRIMARIA
                : COR_TEXTO_SECUNDARIO,
            LV_PART_MAIN
        );


        lv_obj_set_style_text_font(
            lbl,
            &lv_font_montserrat_14,
            LV_PART_MAIN
        );


        lv_label_set_long_mode(
            lbl,
            LV_LABEL_LONG_CLIP
        );


        lv_obj_set_width(
            lbl,
            larguraAba - 4
        );


        lv_obj_set_style_text_align(
            lbl,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN
        );


        lv_obj_center(
            lbl
        );


        lv_obj_remove_flag(
            lbl,
            LV_OBJ_FLAG_CLICKABLE
        );
    }
}


// ============================================================
// TELA INICIAL
// ============================================================

static void cb_status_inicial(
    lv_event_t *e
)
{
    carregarTela(
        TELA_BLOQUEIO
    );
}


void mostrarTelaInicial()
{
    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "MESA PLANA",
        false,
        NULL
    );


    lv_obj_t *cadeado =
        criarCadeado(
            tela,
            60,
            equipamentoBloqueado,
            cb_status_inicial
        );


    lv_obj_align(
        cadeado,
        LV_ALIGN_TOP_MID,
        0,
        28
    );


    lv_obj_t *lblStatus =
        lv_label_create(tela);


    lv_label_set_text(
        lblStatus,
        equipamentoBloqueado
            ? "BLOQUEADO"
            : "LIBERADO"
    );


    lv_obj_set_style_text_color(
        lblStatus,
        equipamentoBloqueado
            ? COR_PERIGO
            : COR_SUCESSO,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblStatus,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblStatus,
        LV_ALIGN_TOP_MID,
        0,
        96
    );


    lv_obj_t *lblToque =
        lv_label_create(tela);


    lv_label_set_text(
        lblToque,
        "toque para controlar"
    );


    lv_obj_set_style_text_color(
        lblToque,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblToque,
        LV_ALIGN_TOP_MID,
        0,
        114
    );


    lv_obj_t *chipTrab =
        criarPainel(
            tela,
            15,
            134,
            290,
            24,
            quantidadeTrabalhadoresAtivos > 0
                ? COR_ALERTA
                : COR_SUPERFICIE_ALTERNATIVA
        );


    lv_obj_set_style_pad_all(
        chipTrab,
        0,
        LV_PART_MAIN
    );


    lv_obj_add_flag(
        chipTrab,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_add_event_cb(
        chipTrab,
        cb_ir_trabalhadores,
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *lblTrabChip =
        lv_label_create(chipTrab);


    lv_label_set_text_fmt(
        lblTrabChip,
        "ATIVOS: %d",
        quantidadeTrabalhadoresAtivos
    );


    lv_obj_set_style_text_color(
        lblTrabChip,
        quantidadeTrabalhadoresAtivos > 0
            ? COR_ALERTA
            : COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_center(
        lblTrabChip
    );


    lv_obj_remove_flag(
        lblTrabChip,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *chipRfid =
        criarPainel(
            tela,
            15,
            162,
            290,
            24,
            COR_PRIMARIA
        );


    lv_obj_set_style_pad_all(
        chipRfid,
        0,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        chipRfid,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *lblRfid =
        lv_label_create(chipRfid);


    lv_label_set_text(
        lblRfid,
        pn532Disponivel
            ? "RFID PRONTO"
            : "RFID NAO DETECTADO"
    );


    lv_obj_set_style_text_color(
        lblRfid,
        pn532Disponivel
            ? COR_PRIMARIA
            : COR_PERIGO,
        LV_PART_MAIN
    );


    lv_obj_center(
        lblRfid
    );


    lv_obj_remove_flag(
        lblRfid,
        LV_OBJ_FLAG_CLICKABLE
    );


    criarNavBar(
        tela,
        0
    );
}


// ============================================================
// MODAL TRABALHADOR
// ============================================================

static void cb_fechar_overlay_bg(
    lv_event_t *e
)
{
    lv_obj_t *overlay =
        (lv_obj_t *)
        lv_event_get_current_target(e);


    lv_obj_del(
        overlay
    );
}


static void cb_fechar_overlay_btn(
    lv_event_t *e
)
{
    lv_obj_t *overlay =
        (lv_obj_t *)
        lv_event_get_user_data(e);


    lv_obj_del(
        overlay
    );
}


static void criarLinhaModal(
    lv_obj_t *parent,
    const char *rotulo,
    const char *valor,
    int y
)
{
    lv_obj_t *lblRot =
        lv_label_create(parent);


    lv_label_set_text(
        lblRot,
        rotulo
    );


    lv_obj_set_style_text_color(
        lblRot,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_set_pos(
        lblRot,
        4,
        y
    );


    lv_obj_t *lblVal =
        lv_label_create(parent);


    lv_label_set_text(
        lblVal,
        valor
    );


    lv_obj_set_style_text_color(
        lblVal,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblVal,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_set_pos(
        lblVal,
        4,
        y + 15
    );
}


void abrirModalTrabalhador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= quantidadeTrabalhadoresCadastrados
    )
        return;


    Trabalhador *t =
        &cadastroTrabalhadores[indice];


    lv_obj_t *tela =
        lv_screen_active();


    lv_obj_t *overlay =
        lv_obj_create(tela);


    lv_obj_remove_style_all(
        overlay
    );


    lv_obj_set_size(
        overlay,
        LARGURA_TELA,
        ALTURA_TELA
    );


    lv_obj_set_pos(
        overlay,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        overlay,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        overlay,
        LV_OPA_70,
        LV_PART_MAIN
    );


    lv_obj_add_flag(
        overlay,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_remove_flag(
        overlay,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_add_event_cb(
        overlay,
        cb_fechar_overlay_bg,
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *card =
        criarPainel(
            overlay,
            15,
            20,
            290,
            200,
            COR_PRIMARIA
        );


    lv_obj_set_style_border_opa(
        card,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        card,
        2,
        LV_PART_MAIN
    );


    lv_obj_align(
        card,
        LV_ALIGN_CENTER,
        0,
        0
    );


    lv_obj_t *lblNome =
        lv_label_create(card);


    lv_label_set_text(
        lblNome,
        t->nome
    );


    lv_obj_set_style_text_color(
        lblNome,
        COR_PRIMARIA,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblNome,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_label_set_long_mode(
        lblNome,
        LV_LABEL_LONG_WRAP
    );


    lv_obj_set_width(
        lblNome,
        230
    );


    lv_obj_set_pos(
        lblNome,
        8,
        10
    );


    criarLinhaModal(
        card,
        "FUNCAO",
        t->funcao,
        56
    );


    criarLinhaModal(
        card,
        "CHAPA / ID RFID",
        t->uidCartao,
        100
    );


    criarLinhaModal(
        card,
        "EMPRESA",
        t->empresa,
        144
    );


    lv_obj_t *btnFechar =
        lv_button_create(card);


    lv_obj_set_size(
        btnFechar,
        30,
        30
    );


    lv_obj_align(
        btnFechar,
        LV_ALIGN_TOP_RIGHT,
        0,
        -4
    );


    lv_obj_set_style_bg_color(
        btnFechar,
        COR_SUPERFICIE_ALTERNATIVA,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        btnFechar,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        btnFechar,
        8,
        LV_PART_MAIN
    );


    lv_obj_set_style_shadow_width(
        btnFechar,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        btnFechar,
        0,
        LV_PART_MAIN
    );


    lv_obj_add_event_cb(
        btnFechar,
        cb_fechar_overlay_btn,
        LV_EVENT_CLICKED,
        overlay
    );


    lv_obj_t *lblX =
        lv_label_create(btnFechar);


    lv_label_set_text(
        lblX,
        "X"
    );


    lv_obj_set_style_text_color(
        lblX,
        COR_PERIGO,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblX,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_center(
        lblX
    );


    lv_obj_remove_flag(
        lblX,
        LV_OBJ_FLAG_CLICKABLE
    );
}


// ============================================================
// FEEDBACK VISUAL RFID
// ============================================================

static lv_obj_t *overlayRFID = nullptr;
static lv_obj_t *modalRFID = nullptr;


void fecharLoadingRFID()
{
    if (overlayRFID != nullptr)
    {
        lv_obj_del(overlayRFID);
        overlayRFID = nullptr;
    }
}


void mostrarLoadingRFID(
    const char *texto
)
{
    fecharLoadingRFID();


    overlayRFID =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_remove_style_all(
        overlayRFID
    );


    lv_obj_set_size(
        overlayRFID,
        LARGURA_TELA,
        ALTURA_TELA
    );


    lv_obj_set_pos(
        overlayRFID,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        overlayRFID,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        overlayRFID,
        LV_OPA_50,
        LV_PART_MAIN
    );


    lv_obj_add_flag(
        overlayRFID,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *card =
        lv_obj_create(
            overlayRFID
        );


    lv_obj_remove_style_all(
        card
    );


    lv_obj_set_size(
        card,
        240,
        120
    );


    lv_obj_center(
        card
    );


    lv_obj_set_style_bg_color(
        card,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        card,
        18,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        card,
        2,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        card,
        COR_PRIMARIA,
        LV_PART_MAIN
    );


    lv_obj_t *spinner =
        lv_spinner_create(card);


    lv_obj_set_size(
        spinner,
        42,
        42
    );


    lv_obj_align(
        spinner,
        LV_ALIGN_TOP_MID,
        0,
        12
    );


    lv_spinner_set_anim_params(
        spinner,
        900,
        180
    );


    lv_obj_t *label =
        lv_label_create(card);


    lv_label_set_text(
        label,
        texto
    );


    lv_obj_set_style_text_color(
        label,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        label,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        label,
        LV_ALIGN_BOTTOM_MID,
        0,
        -17
    );
}


static void fecharMensagemRFIDTimer(
    lv_timer_t *timer
)
{
    lv_obj_t *obj =
        (lv_obj_t *)
        lv_timer_get_user_data(timer);


    if (obj != nullptr)
        lv_obj_del(obj);


    lv_timer_delete(timer);


    carregarTela(
        telaAtual
    );
}


void mostrarMensagemRFID(
    const char *titulo,
    const char *nome,
    const char *subtitulo,
    lv_color_t cor
)
{
    fecharLoadingRFID();


    lv_obj_t *overlay =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_remove_style_all(
        overlay
    );


    lv_obj_set_size(
        overlay,
        LARGURA_TELA,
        ALTURA_TELA
    );


    lv_obj_set_pos(
        overlay,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        overlay,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        overlay,
        LV_OPA_50,
        LV_PART_MAIN
    );


    lv_obj_t *card =
        lv_obj_create(
            overlay
        );


    lv_obj_remove_style_all(
        card
    );


    lv_obj_set_size(
        card,
        270,
        145
    );


    lv_obj_center(
        card
    );


    lv_obj_set_style_bg_color(
        card,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        card,
        18,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        card,
        2,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        card,
        cor,
        LV_PART_MAIN
    );


    lv_obj_t *lblTitulo =
        lv_label_create(card);


    lv_label_set_text(
        lblTitulo,
        titulo
    );


    lv_obj_set_style_text_color(
        lblTitulo,
        cor,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblTitulo,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblTitulo,
        LV_ALIGN_TOP_MID,
        0,
        15
    );


    lv_obj_t *lblNome =
        lv_label_create(card);


    lv_label_set_text(
        lblNome,
        nome
    );


    lv_obj_set_style_text_color(
        lblNome,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblNome,
        LV_ALIGN_CENTER,
        0,
        -2
    );


    lv_obj_t *lblSub =
        lv_label_create(card);


    lv_label_set_text(
        lblSub,
        subtitulo
    );


    lv_obj_set_style_text_color(
        lblSub,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblSub,
        LV_ALIGN_BOTTOM_MID,
        0,
        -16
    );


    lv_timer_create(
        fecharMensagemRFIDTimer,
        1600,
        overlay
    );
}


// ============================================================
// REMOÇÃO RFID
// ============================================================

static void cb_confirmar_remocao(
    lv_event_t *e
)
{
    int indice =
        (int)(intptr_t)
        lv_event_get_user_data(e);


    if (modalRFID != nullptr)
    {
        lv_obj_del(modalRFID);
        modalRFID = nullptr;
    }


    confirmarRemocaoRFID(
        indice
    );
}


static void cb_cancelar_remocao(
    lv_event_t *e
)
{
    cancelarRemocaoRFID();
}


void fecharConfirmacaoRFID()
{
    if (modalRFID != nullptr)
    {
        lv_obj_del(modalRFID);
        modalRFID = nullptr;
    }
}


bool mostrarConfirmacaoRemocaoRFID(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= quantidadeTrabalhadoresCadastrados
    )
        return false;


    fecharConfirmacaoRFID();


    Trabalhador *t =
        &cadastroTrabalhadores[indice];


    modalRFID =
        lv_obj_create(
            lv_screen_active()
        );

    // Protecao contra esgotamento/fragmentacao da memoria interna do
    // LVGL (LV_MEM_SIZE). Sem essa checagem, um lv_obj_create() que
    // falhe (retorna NULL) faria as chamadas seguintes (lv_obj_set_size
    // etc.) desreferenciarem um ponteiro nulo dentro do LVGL - isso
    // trava/reinicia o ESP32. Se falhar, desiste de mostrar o popup em
    // vez de arriscar um crash.
    if (modalRFID == nullptr)
    {
        Serial.println(
            "ERRO: falha ao criar modal de confirmacao (memoria LVGL insuficiente)"
        );

        return false;
    }


    lv_obj_remove_style_all(
        modalRFID
    );


    lv_obj_set_size(
        modalRFID,
        LARGURA_TELA,
        ALTURA_TELA
    );


    lv_obj_set_pos(
        modalRFID,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        modalRFID,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        modalRFID,
        LV_OPA_50,
        LV_PART_MAIN
    );


    lv_obj_t *card =
        lv_obj_create(
            modalRFID
        );

    if (card == nullptr)
    {
        Serial.println(
            "ERRO: falha ao criar card do modal (memoria LVGL insuficiente)"
        );

        lv_obj_del(modalRFID);
        modalRFID = nullptr;

        return false;
    }


    lv_obj_remove_style_all(
        card
    );


    lv_obj_set_size(
        card,
        290,
        165
    );


    lv_obj_center(
        card
    );


    lv_obj_set_style_bg_color(
        card,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        card,
        18,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        card,
        2,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        card,
        COR_ALERTA,
        LV_PART_MAIN
    );


    lv_obj_t *titulo =
        lv_label_create(card);


    lv_label_set_text(
        titulo,
        "DESEJA REMOVER?"
    );


    lv_obj_set_style_text_color(
        titulo,
        COR_ALERTA,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        titulo,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        titulo,
        LV_ALIGN_TOP_MID,
        0,
        14
    );


    lv_obj_t *nome =
        lv_label_create(card);


    lv_label_set_text(
        nome,
        t->nome
    );


    lv_obj_set_style_text_color(
        nome,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        nome,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        nome,
        LV_ALIGN_CENTER,
        0,
        -18
    );


    lv_obj_t *pergunta =
        lv_label_create(card);


    lv_label_set_text(
        pergunta,
        "Deseja retirar da linha?"
    );


    lv_obj_set_style_text_color(
        pergunta,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        pergunta,
        LV_ALIGN_CENTER,
        0,
        8
    );


    criarBotao(
        card,
        "NAO",
        18,
        112,
        115,
        36,
        COR_SUPERFICIE_ALTERNATIVA,
        cb_cancelar_remocao
    );


    lv_obj_t *btnSim =
        criarBotao(
            card,
            "SIM",
            157,
            112,
            115,
            36,
            COR_PERIGO,
            NULL
        );


    lv_obj_add_event_cb(
        btnSim,
        cb_confirmar_remocao,
        LV_EVENT_CLICKED,
        (void *)(intptr_t)indice
    );

    return true;
}


// ============================================================
// TRABALHADORES
// ============================================================

static void cb_abrir_trabalhador(
    lv_event_t *e
)
{
    int indice =
        (int)(intptr_t)
        lv_event_get_user_data(e);


    abrirModalTrabalhador(
        indice
    );
}


void mostrarTrabalhadores()
{
    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "EQUIPE",
        false,
        NULL
    );


    lv_obj_t *contadorEquipe =
        lv_label_create(tela);


    lv_label_set_text_fmt(
        contadorEquipe,
        "ATIVOS: %d",
        quantidadeTrabalhadoresAtivos
    );


    lv_obj_set_style_text_color(
        contadorEquipe,
        COR_PRIMARIA,
        LV_PART_MAIN
    );


    lv_obj_align(
        contadorEquipe,
        LV_ALIGN_TOP_RIGHT,
        -10,
        7
    );


    lv_obj_t *lista =
        criarPainel(
            tela,
            15,
            38,
            290,
            146,
            COR_SUPERFICIE_ALTERNATIVA
        );


    lv_obj_set_flex_flow(
        lista,
        LV_FLEX_FLOW_COLUMN
    );


    lv_obj_set_flex_align(
        lista,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START
    );


    lv_obj_set_style_pad_all(
        lista,
        6,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_row(
        lista,
        5,
        LV_PART_MAIN
    );


    lv_obj_add_flag(
        lista,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_scroll_dir(
        lista,
        LV_DIR_VER
    );


    if (
        quantidadeTrabalhadoresAtivos == 0
    )
    {
        lv_obj_t *vazio =
            lv_label_create(lista);


        lv_label_set_text(
            vazio,
            "Nenhum trabalhador registrado"
        );


        lv_obj_set_style_text_color(
            vazio,
            COR_TEXTO_SECUNDARIO,
            LV_PART_MAIN
        );
    }
    else
    {
        for (
            int i = 0;
            i < quantidadeTrabalhadoresCadastrados;
            i++
        )
        {
            if (
                !cadastroTrabalhadores[i].ativo
            )
            {
                continue;
            }


            lv_obj_t *item =
                lv_button_create(lista);


            lv_obj_set_size(
                item,
                LV_PCT(100),
                30
            );


            lv_obj_set_style_bg_color(
                item,
                COR_FUNDO,
                LV_PART_MAIN
            );


            lv_obj_set_style_bg_opa(
                item,
                LV_OPA_COVER,
                LV_PART_MAIN
            );


            lv_obj_set_style_radius(
                item,
                8,
                LV_PART_MAIN
            );


            lv_obj_set_style_border_width(
                item,
                1,
                LV_PART_MAIN
            );


            lv_obj_set_style_border_color(
                item,
                COR_PRIMARIA,
                LV_PART_MAIN
            );


            lv_obj_set_style_border_opa(
                item,
                LV_OPA_50,
                LV_PART_MAIN
            );


            lv_obj_set_style_shadow_width(
                item,
                0,
                LV_PART_MAIN
            );


            lv_obj_remove_flag(
                item,
                LV_OBJ_FLAG_SCROLLABLE
            );


            lv_obj_add_event_cb(
                item,
                cb_abrir_trabalhador,
                LV_EVENT_CLICKED,
                (void *)(intptr_t)i
            );


            lv_obj_t *lblNome =
                lv_label_create(item);


            lv_label_set_text(
                lblNome,
                cadastroTrabalhadores[i].nome
            );


            lv_obj_set_style_text_color(
                lblNome,
                COR_TEXTO_PRINCIPAL,
                LV_PART_MAIN
            );


            lv_label_set_long_mode(
                lblNome,
                LV_LABEL_LONG_DOT
            );


            lv_obj_set_width(
                lblNome,
                LV_PCT(94)
            );


            lv_obj_align(
                lblNome,
                LV_ALIGN_LEFT_MID,
                8,
                0
            );


            lv_obj_remove_flag(
                lblNome,
                LV_OBJ_FLAG_CLICKABLE
            );


        }
    }


    criarNavBar(
        tela,
        1
    );
}


// ============================================================
// BLOQUEIO
// ============================================================

static void cb_bloquear(
    lv_event_t *e
)
{
    equipamentoBloqueado =
        true;


    atualizarRele();


    mostrarBloqueio();
}


static void cb_liberar(
    lv_event_t *e
)
{
    if (
        quantidadeTrabalhadoresAtivos == 0
    )
    {
        equipamentoBloqueado =
            false;


        atualizarRele();
    }
    else
    {
        equipamentoBloqueado =
            true;


        atualizarRele();
    }


    mostrarBloqueio();
}


void mostrarBloqueio()
{
    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "CONTROLE DE BLOQUEIO",
        false,
        NULL
    );


    lv_obj_t *cadeado =
        criarCadeado(
            tela,
            44,
            equipamentoBloqueado,
            NULL
        );


    lv_obj_align(
        cadeado,
        LV_ALIGN_TOP_MID,
        0,
        31
    );


    lv_obj_t *lblStatus =
        lv_label_create(tela);


    lv_label_set_text(
        lblStatus,
        equipamentoBloqueado
            ? "BLOQUEADO"
            : "LIBERADO"
    );


    lv_obj_set_style_text_color(
        lblStatus,
        equipamentoBloqueado
            ? COR_PERIGO
            : COR_SUCESSO,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        lblStatus,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblStatus,
        LV_ALIGN_TOP_MID,
        0,
        79
    );


    lv_obj_t *lblTrab =
        lv_label_create(tela);


    lv_label_set_text_fmt(
        lblTrab,
        "Trabalhadores na linha: %d",
        quantidadeTrabalhadoresAtivos
    );


    lv_obj_set_style_text_color(
        lblTrab,
        quantidadeTrabalhadoresAtivos > 0
            ? COR_ALERTA
            : COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblTrab,
        LV_ALIGN_TOP_MID,
        0,
        98
    );


    lv_obj_t *pill =
        lv_obj_create(tela);


    lv_obj_set_size(
        pill,
        290,
        34
    );


    lv_obj_align(
        pill,
        LV_ALIGN_TOP_MID,
        0,
        118
    );


    lv_obj_set_style_bg_color(
        pill,
        COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        pill,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        pill,
        17,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        pill,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_all(
        pill,
        2,
        LV_PART_MAIN
    );


    lv_obj_remove_flag(
        pill,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_remove_flag(
        pill,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *ladoBloquear =
        lv_button_create(pill);


    lv_obj_set_size(
        ladoBloquear,
        141,
        30
    );


    lv_obj_set_pos(
        ladoBloquear,
        0,
        0
    );


    lv_obj_set_style_radius(
        ladoBloquear,
        15,
        LV_PART_MAIN
    );


    lv_obj_set_style_shadow_width(
        ladoBloquear,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        ladoBloquear,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_color(
        ladoBloquear,
        equipamentoBloqueado
            ? COR_PERIGO
            : COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_add_event_cb(
        ladoBloquear,
        cb_bloquear,
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *lblBloquear =
        lv_label_create(
            ladoBloquear
        );


    lv_label_set_text(
        lblBloquear,
        "BLOQUEAR"
    );


    lv_obj_set_style_text_color(
        lblBloquear,
        equipamentoBloqueado
            ? COR_TEXTO_SOBRE_DESTAQUE
            : COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_center(
        lblBloquear
    );


    lv_obj_remove_flag(
        lblBloquear,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *ladoLiberar =
        lv_button_create(pill);


    lv_obj_set_size(
        ladoLiberar,
        141,
        30
    );


    lv_obj_set_pos(
        ladoLiberar,
        147,
        0
    );


    lv_obj_set_style_radius(
        ladoLiberar,
        15,
        LV_PART_MAIN
    );


    lv_obj_set_style_shadow_width(
        ladoLiberar,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        ladoLiberar,
        0,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_color(
        ladoLiberar,
        (!equipamentoBloqueado)
            ? COR_SUCESSO
            : COR_SUPERFICIE,
        LV_PART_MAIN
    );


    lv_obj_add_event_cb(
        ladoLiberar,
        cb_liberar,
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *lblLiberar =
        lv_label_create(
            ladoLiberar
        );


    lv_label_set_text(
        lblLiberar,
        "LIBERAR"
    );


    lv_obj_set_style_text_color(
        lblLiberar,
        (!equipamentoBloqueado)
            ? COR_TEXTO_SOBRE_DESTAQUE
            : COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_center(
        lblLiberar
    );


    lv_obj_remove_flag(
        lblLiberar,
        LV_OBJ_FLAG_CLICKABLE
    );


    if (
        quantidadeTrabalhadoresAtivos > 0
    )
    {
        lv_obj_t *lblWarn =
            lv_label_create(tela);


        lv_label_set_text(
            lblWarn,
            "Nao e possivel liberar com equipe ativa"
        );


        lv_obj_set_style_text_color(
            lblWarn,
            COR_ALERTA,
            LV_PART_MAIN
        );


        lv_obj_set_style_text_font(
            lblWarn,
            &lv_font_montserrat_14,
            LV_PART_MAIN
        );


        lv_obj_align(
            lblWarn,
            LV_ALIGN_TOP_MID,
            0,
            160
        );
    }


    criarNavBar(
        tela,
        2
    );
}


// ============================================================
// WI-FI
// ============================================================

static lv_obj_t *wifiSsidTA = nullptr;
static lv_obj_t *wifiSenhaTA = nullptr;
static lv_obj_t *wifiStatusLbl = nullptr;

// NOVO:
// Label que mostra o IP do ESP32.
static lv_obj_t *wifiIpLbl = nullptr;

// NOVO:
// Label que mostra o endereço do servidor (backend).
static lv_obj_t *wifiJsonLbl = nullptr;

static lv_obj_t *wifiKeyboard = nullptr;


// ------------------------------------------------------------
// FECHAR TECLADO
// ------------------------------------------------------------

static void fecharTecladoWiFi()
{
    if (wifiKeyboard)
    {
        lv_obj_del(
            wifiKeyboard
        );

        wifiKeyboard = nullptr;
    }
}


// ------------------------------------------------------------
// SAIR DA TELA WI-FI
// ------------------------------------------------------------

static void cb_wifi_sair(
    lv_event_t *e
)
{
    fecharTecladoWiFi();


    wifiSsidTA = nullptr;
    wifiSenhaTA = nullptr;
    wifiStatusLbl = nullptr;
    wifiIpLbl = nullptr;
    wifiJsonLbl = nullptr;


    carregarTela(
        TELA_MAIS
    );
}


// ------------------------------------------------------------
// TECLADO WI-FI
// ------------------------------------------------------------

static void cb_wifi_textarea(
    lv_event_t *e
)
{
    lv_obj_t *ta =
        (lv_obj_t *)
        lv_event_get_target(e);


    if (!wifiKeyboard)
    {
        wifiKeyboard =
            lv_keyboard_create(
                lv_screen_active()
            );
    }


    lv_keyboard_set_textarea(
        wifiKeyboard,
        ta
    );


    lv_obj_set_size(
        wifiKeyboard,
        LARGURA_TELA,
        78
    );


    lv_obj_align(
        wifiKeyboard,
        LV_ALIGN_BOTTOM_MID,
        0,
        0
    );
}


// ------------------------------------------------------------
// CONECTAR WI-FI
// ------------------------------------------------------------

static void cb_wifi_conectar(
    lv_event_t *e
)
{
    const char *ssid =
        wifiSsidTA
            ? lv_textarea_get_text(
                wifiSsidTA
              )
            : "";


    const char *senha =
        wifiSenhaTA
            ? lv_textarea_get_text(
                wifiSenhaTA
              )
            : "";


    if (
        !ssid ||
        ssid[0] == '\0'
    )
    {
        if (wifiStatusLbl)
        {
            lv_label_set_text(
                wifiStatusLbl,
                "Informe o nome da rede."
            );
        }

        return;
    }


    salvarConfiguracaoWiFi(
        ssid,
        senha
    );


    fecharTecladoWiFi();


    conectarWiFi();


    if (wifiStatusLbl)
    {
        lv_label_set_text(
            wifiStatusLbl,
            "Conectando ao Wi-Fi..."
        );


        lv_obj_set_style_text_color(
            wifiStatusLbl,
            COR_ALERTA,
            LV_PART_MAIN
        );
    }


    if (wifiIpLbl)
    {
        lv_label_set_text(
            wifiIpLbl,
            "IP: aguardando conexao..."
        );
    }


    if (wifiJsonLbl)
    {
        lv_label_set_text(
            wifiJsonLbl,
            ""
        );
    }
}


// ------------------------------------------------------------
// ATUALIZAR STATUS WI-FI NA TELA
// ------------------------------------------------------------

void atualizarStatusWiFiTela()
{
    if (!wifiStatusLbl)
        return;


    if (wifiEstaConectado())
    {
        // ----------------------------------------------------
        // WIFI CONECTADO
        // ----------------------------------------------------

        lv_label_set_text(
            wifiStatusLbl,
            "Wi-Fi conectado"
        );


        lv_obj_set_style_text_color(
            wifiStatusLbl,
            COR_SUCESSO,
            LV_PART_MAIN
        );


        // ----------------------------------------------------
        // IP
        // ----------------------------------------------------

        if (wifiIpLbl)
        {
            String ip =
                WiFi.localIP().toString();


            String textoIP =
                "IP: " + ip;


            lv_label_set_text(
                wifiIpLbl,
                textoIP.c_str()
            );


            lv_obj_set_style_text_color(
                wifiIpLbl,
                COR_PRIMARIA,
                LV_PART_MAIN
            );
        }


        // ----------------------------------------------------
        // SERVIDOR (backend que recebe os eventos)
        // ----------------------------------------------------

        if (wifiJsonLbl)
        {
            String url =
                String("Servidor: ") +
                SERVIDOR_URL;


            lv_label_set_text(
                wifiJsonLbl,
                url.c_str()
            );


            lv_obj_set_style_text_color(
                wifiJsonLbl,
                COR_TEXTO_SECUNDARIO,
                LV_PART_MAIN
            );
        }
    }
    else if (
        obterWiFiSSID()[0]
    )
    {
        // ----------------------------------------------------
        // CONFIGURADO / CONECTANDO
        // ----------------------------------------------------

        lv_label_set_text(
            wifiStatusLbl,
            "Conectando ao Wi-Fi..."
        );


        lv_obj_set_style_text_color(
            wifiStatusLbl,
            COR_ALERTA,
            LV_PART_MAIN
        );


        if (wifiIpLbl)
        {
            lv_label_set_text(
                wifiIpLbl,
                "IP: aguardando conexao..."
            );


            lv_obj_set_style_text_color(
                wifiIpLbl,
                COR_TEXTO_SECUNDARIO,
                LV_PART_MAIN
            );
        }


        if (wifiJsonLbl)
        {
            lv_label_set_text(
                wifiJsonLbl,
                ""
            );
        }
    }
    else
    {
        // ----------------------------------------------------
        // NENHUMA REDE CONFIGURADA
        // ----------------------------------------------------

        lv_label_set_text(
            wifiStatusLbl,
            "Nenhuma rede configurada"
        );


        lv_obj_set_style_text_color(
            wifiStatusLbl,
            COR_TEXTO_SECUNDARIO,
            LV_PART_MAIN
        );


        if (wifiIpLbl)
        {
            lv_label_set_text(
                wifiIpLbl,
                "IP: --"
            );


            lv_obj_set_style_text_color(
                wifiIpLbl,
                COR_TEXTO_SECUNDARIO,
                LV_PART_MAIN
            );
        }


        if (wifiJsonLbl)
        {
            lv_label_set_text(
                wifiJsonLbl,
                "Servidor: --"
            );
        }
    }
}


// ------------------------------------------------------------
// TELA DE WI-FI
// ------------------------------------------------------------

void mostrarWiFi()
{
    fecharTecladoWiFi();


    wifiSsidTA = nullptr;
    wifiSenhaTA = nullptr;
    wifiStatusLbl = nullptr;
    wifiIpLbl = nullptr;
    wifiJsonLbl = nullptr;


    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "CONFIGURAR WI-FI",
        true,
        cb_wifi_sair
    );


    // --------------------------------------------------------
    // REDE
    // --------------------------------------------------------

    lv_obj_t *lbl1 =
        lv_label_create(tela);


    lv_label_set_text(
        lbl1,
        "REDE WI-FI"
    );


    lv_obj_set_style_text_color(
        lbl1,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lbl1,
        LV_ALIGN_TOP_LEFT,
        18,
        38
    );


    wifiSsidTA =
        lv_textarea_create(tela);


    lv_obj_set_size(
        wifiSsidTA,
        284,
        36
    );


    lv_obj_align(
        wifiSsidTA,
        LV_ALIGN_TOP_LEFT,
        18,
        50
    );


    lv_textarea_set_one_line(
        wifiSsidTA,
        true
    );


    lv_textarea_set_placeholder_text(
        wifiSsidTA,
        "Nome da rede"
    );


    lv_textarea_set_text(
        wifiSsidTA,
        obterWiFiSSID()
    );


    lv_obj_add_event_cb(
        wifiSsidTA,
        cb_wifi_textarea,
        LV_EVENT_FOCUSED,
        nullptr
    );


    // --------------------------------------------------------
    // SENHA
    // --------------------------------------------------------

    lv_obj_t *lbl2 =
        lv_label_create(tela);


    lv_label_set_text(
        lbl2,
        "SENHA"
    );


    lv_obj_set_style_text_color(
        lbl2,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lbl2,
        LV_ALIGN_TOP_LEFT,
        18,
        88
    );


    wifiSenhaTA =
        lv_textarea_create(tela);


    lv_obj_set_size(
        wifiSenhaTA,
        284,
        36
    );


    lv_obj_align(
        wifiSenhaTA,
        LV_ALIGN_TOP_LEFT,
        18,
        100
    );


    lv_textarea_set_one_line(
        wifiSenhaTA,
        true
    );


    lv_textarea_set_password_mode(
        wifiSenhaTA,
        true
    );


    lv_textarea_set_placeholder_text(
        wifiSenhaTA,
        "Senha do Wi-Fi"
    );


    lv_textarea_set_text(
        wifiSenhaTA,
        obterWiFiSenha()
    );


    lv_obj_add_event_cb(
        wifiSenhaTA,
        cb_wifi_textarea,
        LV_EVENT_FOCUSED,
        nullptr
    );


    // --------------------------------------------------------
    // BOTAO
    // --------------------------------------------------------

    criarBotao(
        tela,
        "SALVAR E CONECTAR",
        18,
        140,
        284,
        34,
        COR_PRIMARIA,
        cb_wifi_conectar
    );


    // --------------------------------------------------------
    // STATUS
    // --------------------------------------------------------

    wifiStatusLbl =
        lv_label_create(tela);


    lv_obj_set_style_text_font(
        wifiStatusLbl,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        wifiStatusLbl,
        LV_ALIGN_TOP_MID,
        0,
        178
    );


    // --------------------------------------------------------
    // IP
    // --------------------------------------------------------

    wifiIpLbl =
        lv_label_create(tela);


    lv_label_set_text(
        wifiIpLbl,
        "IP: --"
    );


    lv_obj_set_style_text_font(
        wifiIpLbl,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        wifiIpLbl,
        LV_ALIGN_TOP_MID,
        0,
        196
    );


    // --------------------------------------------------------
    // JSON
    // --------------------------------------------------------

    wifiJsonLbl =
        lv_label_create(tela);


    lv_label_set_text(
        wifiJsonLbl,
        "Servidor: --"
    );


    lv_obj_set_style_text_font(
        wifiJsonLbl,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_align(
        wifiJsonLbl,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN
    );


    lv_obj_set_width(
        wifiJsonLbl,
        310
    );


    lv_obj_align(
        wifiJsonLbl,
        LV_ALIGN_TOP_MID,
        0,
        214
    );


    // --------------------------------------------------------
    // ATUALIZA TUDO CONFORME ESTADO ATUAL
    // --------------------------------------------------------

    atualizarStatusWiFiTela();
}


// ============================================================
// MAIS OPÇÕES
// ============================================================

void mostrarMais()
{
    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "MAIS OPCOES",
        false,
        NULL
    );


    // --------------------------------------------------------
    // HISTORICO
    // --------------------------------------------------------

    lv_obj_t *opHist =
        criarPainel(
            tela,
            15,
            40,
            290,
            52,
            COR_PRIMARIA
        );


    lv_obj_add_flag(
        opHist,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_add_event_cb(
        opHist,
        cb_ir_historico,
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *lblHistT =
        lv_label_create(opHist);


    lv_label_set_text(
        lblHistT,
        "Historico de registros"
    );


    lv_obj_set_style_text_color(
        lblHistT,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblHistT,
        LV_ALIGN_LEFT_MID,
        4,
        -8
    );


    lv_obj_remove_flag(
        lblHistT,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *lblHistS =
        lv_label_create(opHist);


    lv_label_set_text(
        lblHistS,
        "eventos LOTO recentes"
    );


    lv_obj_set_style_text_color(
        lblHistS,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblHistS,
        LV_ALIGN_LEFT_MID,
        4,
        10
    );


    lv_obj_remove_flag(
        lblHistS,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *setaHist =
        lv_label_create(opHist);


    lv_label_set_text(
        setaHist,
        ">"
    );


    lv_obj_set_style_text_color(
        setaHist,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        setaHist,
        LV_ALIGN_RIGHT_MID,
        -6,
        0
    );


    lv_obj_remove_flag(
        setaHist,
        LV_OBJ_FLAG_CLICKABLE
    );


    // --------------------------------------------------------
    // WI-FI
    // --------------------------------------------------------

    lv_obj_t *opWifi =
        criarPainel(
            tela,
            15,
            100,
            290,
            52,
            COR_PRIMARIA
        );


    lv_obj_add_flag(
        opWifi,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_add_event_cb(
        opWifi,
        [](lv_event_t *e)
        {
            mostrarWiFi();
        },
        LV_EVENT_CLICKED,
        NULL
    );


    lv_obj_t *lblWifiT =
        lv_label_create(opWifi);


    lv_label_set_text(
        lblWifiT,
        "Configurar Wi-Fi"
    );


    lv_obj_set_style_text_color(
        lblWifiT,
        COR_TEXTO_PRINCIPAL,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblWifiT,
        LV_ALIGN_LEFT_MID,
        4,
        -8
    );


    lv_obj_remove_flag(
        lblWifiT,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *lblWifiS =
        lv_label_create(opWifi);


    lv_label_set_text(
        lblWifiS,
        "Rede e senha pela tela"
    );


    lv_obj_set_style_text_color(
        lblWifiS,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        lblWifiS,
        LV_ALIGN_LEFT_MID,
        4,
        10
    );


    lv_obj_remove_flag(
        lblWifiS,
        LV_OBJ_FLAG_CLICKABLE
    );


    lv_obj_t *setaWifi =
        lv_label_create(opWifi);


    lv_label_set_text(
        setaWifi,
        ">"
    );


    lv_obj_set_style_text_color(
        setaWifi,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        setaWifi,
        LV_ALIGN_RIGHT_MID,
        -6,
        0
    );


    lv_obj_remove_flag(
        setaWifi,
        LV_OBJ_FLAG_CLICKABLE
    );


    criarNavBar(
        tela,
        3
    );
}


// ============================================================
// APAGAR LOGS
// ============================================================

static lv_obj_t *modalApagarLogs = nullptr;


static void fecharModalApagarLogs()
{
    if (
        modalApagarLogs != nullptr
    )
    {
        lv_obj_del(
            modalApagarLogs
        );

        modalApagarLogs = nullptr;
    }
}


void mostrarConfirmacaoApagarLogs()
{
    fecharModalApagarLogs();


    modalApagarLogs =
        lv_obj_create(
            lv_screen_active()
        );


    lv_obj_remove_style_all(
        modalApagarLogs
    );


    lv_obj_set_size(
        modalApagarLogs,
        LARGURA_TELA,
        ALTURA_TELA
    );


    lv_obj_set_pos(
        modalApagarLogs,
        0,
        0
    );


    lv_obj_set_style_bg_color(
        modalApagarLogs,
        lv_color_hex(0x000000),
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        modalApagarLogs,
        LV_OPA_50,
        LV_PART_MAIN
    );


    lv_obj_t *card =
        lv_obj_create(
            modalApagarLogs
        );


    lv_obj_remove_style_all(
        card
    );


    lv_obj_set_size(
        card,
        290,
        145
    );


    lv_obj_center(
        card
    );


    lv_obj_set_style_bg_color(
        card,
        COR_FUNDO,
        LV_PART_MAIN
    );


    lv_obj_set_style_bg_opa(
        card,
        LV_OPA_COVER,
        LV_PART_MAIN
    );


    lv_obj_set_style_radius(
        card,
        18,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_width(
        card,
        2,
        LV_PART_MAIN
    );


    lv_obj_set_style_border_color(
        card,
        COR_PERIGO,
        LV_PART_MAIN
    );


    lv_obj_t *titulo =
        lv_label_create(card);


    lv_label_set_text(
        titulo,
        "APAGAR HISTORICO?"
    );


    lv_obj_set_style_text_color(
        titulo,
        COR_PERIGO,
        LV_PART_MAIN
    );


    lv_obj_set_style_text_font(
        titulo,
        &lv_font_montserrat_14,
        LV_PART_MAIN
    );


    lv_obj_align(
        titulo,
        LV_ALIGN_TOP_MID,
        0,
        14
    );


    lv_obj_t *sub =
        lv_label_create(card);


    lv_label_set_text(
        sub,
        "Todos os logs serao removidos."
    );


    lv_obj_set_style_text_color(
        sub,
        COR_TEXTO_SECUNDARIO,
        LV_PART_MAIN
    );


    lv_obj_align(
        sub,
        LV_ALIGN_CENTER,
        0,
        -5
    );


    lv_obj_t *nao =
        criarBotao(
            card,
            "NAO",
            18,
            95,
            115,
            36,
            COR_SUPERFICIE_ALTERNATIVA,
            [](lv_event_t *e)
            {
                fecharModalApagarLogs();
            }
        );


    lv_obj_t *sim =
        criarBotao(
            card,
            "SIM",
            157,
            95,
            115,
            36,
            COR_PERIGO,
            [](lv_event_t *e)
            {
                fecharModalApagarLogs();

                apagarTodosLogs();

                mostrarHistorico();
            }
        );


    (void)nao;
    (void)sim;
}


// ============================================================
// HISTORICO
// ============================================================

void mostrarHistorico()
{
    limparTela();


    lv_obj_t *tela =
        lv_screen_active();


    criarTopo(
        tela,
        "HISTORICO",
        true,
        cb_ir_mais
    );


    lv_obj_t *lista =
        criarPainel(
            tela,
            10,
            34,
            300,
            150,
            COR_SUPERFICIE_ALTERNATIVA
        );


    lv_obj_set_flex_flow(
        lista,
        LV_FLEX_FLOW_COLUMN
    );


    lv_obj_set_style_pad_all(
        lista,
        7,
        LV_PART_MAIN
    );


    lv_obj_set_style_pad_row(
        lista,
        7,
        LV_PART_MAIN
    );


    lv_obj_add_flag(
        lista,
        LV_OBJ_FLAG_SCROLLABLE
    );


    lv_obj_set_scroll_dir(
        lista,
        LV_DIR_VER
    );


    if (
        quantidadeLogs == 0
    )
    {
        lv_obj_t *vazio =
            lv_label_create(lista);


        lv_label_set_text(
            vazio,
            "Nenhum registro criado."
        );


        lv_obj_set_style_text_color(
            vazio,
            COR_TEXTO_SECUNDARIO,
            LV_PART_MAIN
        );
    }
    else
    {
        int inicio =
            quantidadeLogs > 8
                ? quantidadeLogs - 8
                : 0;


        for (
            int i = inicio;
            i < quantidadeLogs;
            i++
        )
        {
            RegistroLog &l =
                registrosLog[i];


            lv_obj_t *linha =
                lv_obj_create(lista);


            lv_obj_remove_style_all(
                linha
            );


            lv_obj_set_size(
                linha,
                LV_PCT(100),
                42
            );


            lv_obj_set_style_bg_color(
                linha,
                COR_FUNDO,
                LV_PART_MAIN
            );


            lv_obj_set_style_bg_opa(
                linha,
                LV_OPA_COVER,
                LV_PART_MAIN
            );


            lv_obj_set_style_radius(
                linha,
                8,
                LV_PART_MAIN
            );


            lv_obj_t *txt =
                lv_label_create(linha);


            lv_label_set_text_fmt(
                txt,
                "%s %s | %s\n%s | %s",
                l.data,
                l.hora,
                l.evento,
                l.nome,
                l.equipamento
            );


            lv_obj_set_style_text_color(
                txt,
                COR_TEXTO_PRINCIPAL,
                LV_PART_MAIN
            );


            lv_obj_align(
                txt,
                LV_ALIGN_LEFT_MID,
                7,
                0
            );
        }
    }


    criarBotao(
        tela,
        "APAGAR LOGS",
        20,
        190,
        135,
        32,
        COR_PERIGO,
        [](lv_event_t *e)
        {
            mostrarConfirmacaoApagarLogs();
        }
    );


    criarBotao(
        tela,
        "ATUALIZAR",
        165,
        190,
        135,
        32,
        COR_PRIMARIA,
        [](lv_event_t *e)
        {
            mostrarHistorico();
        }
    );
}


// ============================================================
// CARREGAR TELA
// ============================================================

void carregarTela(
    Tela novaTela
)
{
    // Se havia um popup "DESEJA REMOVER?" pendente e a tela for
    // trocada por qualquer outro motivo (ex.: navegacao para outra
    // aba), cancela a confirmacao pelo caminho correto. Isso garante que
    // a flag aguardandoConfirmacao (em RFID.cpp) seja resetada e o
    // leitor RFID nao fique travado sem ler mais nada.
    cancelarRemocaoRFID();

    telaAtual =
        novaTela;


    switch (
        novaTela
    )
    {
        case TELA_INICIAL:

            mostrarTelaInicial();

            break;


        case TELA_TRABALHADORES:

            mostrarTrabalhadores();

            break;


        case TELA_BLOQUEIO:

            mostrarBloqueio();

            break;


        case TELA_MAIS:

            mostrarMais();

            break;


        case TELA_HISTORICO:

            mostrarHistorico();

            break;


        case TELA_WIFI:

            mostrarWiFi();

            break;
    }
}