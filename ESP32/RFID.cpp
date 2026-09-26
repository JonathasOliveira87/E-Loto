#include "RFID.h"
#include "Hardware.h"
#include "Estado.h"
#include "Telas.h"
#include "Logs.h"
#include <Arduino.h>
#include <string.h>
#include <math.h>
#include <esp_task_wdt.h>

void iniciarPN532()
{
    barramentoNfc.begin(
        PIN_PN532_SDA,
        PIN_PN532_SCL,
        VELOCIDADE_I2C_PN532
    );

    delay(300);

    leitorNfc.begin();

    uint32_t versiondata =
        leitorNfc.getFirmwareVersion();


    if (!versiondata)
    {
        pn532Disponivel = false;
        return;
    }


    pn532Disponivel = true;


    uint8_t ic =
        (versiondata >> 24) & 0xFF;

    uint8_t ver =
        (versiondata >> 16) & 0xFF;

    uint8_t rev =
        (versiondata >> 8) & 0xFF;

    leitorNfc.SAMConfig();

    delay(100);

}

// Reinicializa o PN532 do zero. Usado como recuperacao quando o leitor
// para de responder (ex.: depois de uma colisao com 2 cartoes no campo
// ao mesmo tempo, o modulo as vezes fica "surdo" e so volta com um
// SAMConfig novo).
static void reiniciarPN532()
{
    leitorNfc.begin();
    delay(50);
    leitorNfc.SAMConfig();
    delay(50);
}

void uidParaTexto(
    uint8_t *uid,
    uint8_t uidLength,
    char *saida,
    size_t tamanho
)
{
    saida[0] = '\0';


    for (
        uint8_t i = 0;
        i < uidLength;
        i++
    )
    {
        char parte[4];


        snprintf(
            parte,
            sizeof(parte),
            "%02X",
            uid[i]
        );


        strncat(
            saida,
            parte,
            tamanho -
            strlen(saida) -
            1
        );


        if (
            i <
            uidLength - 1
        )
        {
            strncat(
                saida,
                ":",
                tamanho -
                strlen(saida) -
                1
            );
        }
    }
}

int encontrarTrabalhadorRFID(
    const char *uidTexto
)
{
    for (
        int i = 0;
        i < quantidadeTrabalhadoresCadastrados;
        i++
    )
    {
        if (
            cadastroTrabalhadores[i].cadastrado &&
            cadastroTrabalhadores[i].ativo &&
            strcmp(
                cadastroTrabalhadores[i].uidCartao,
                uidTexto
            ) == 0
        )
        {
            return i;
        }
    }


    return -1;
}

int encontrarTrabalhadorCadastrado(
    const char *uidTexto
)
{
    for (
        int i = 0;
        i < quantidadeTrabalhadoresCadastrados;
        i++
    )
    {
        if (
            cadastroTrabalhadores[i].cadastrado &&
            strcmp(
                cadastroTrabalhadores[i].uidCartao,
                uidTexto
            ) == 0
        )
        {
            return i;
        }
    }


    return -1;
}

void atualizarEstadoBloqueio()
{
    if (quantidadeTrabalhadoresAtivos > 0)
    {
        equipamentoBloqueado = true;
    }
    else
    {
        equipamentoBloqueado = false;
    }


    atualizarRele();


    if (equipamentoBloqueado)
    {

    }
    else
    {

    }
}

void ativarTrabalhador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= quantidadeTrabalhadoresCadastrados
    )
        return;


    if (
        !cadastroTrabalhadores[indice].ativo
    )
    {
        cadastroTrabalhadores[indice].ativo =
            true;

        quantidadeTrabalhadoresAtivos++;
    }


    atualizarEstadoBloqueio();
}

void desativarTrabalhador(
    int indice
)
{
    if (
        indice < 0 ||
        indice >= quantidadeTrabalhadoresCadastrados
    )
        return;


    if (
        cadastroTrabalhadores[indice].ativo
    )
    {
        cadastroTrabalhadores[indice].ativo =
            false;


        if (quantidadeTrabalhadoresAtivos > 0)
        {
            quantidadeTrabalhadoresAtivos--;
        }
    }


    atualizarEstadoBloqueio();
}

static bool aguardandoConfirmacao = false;

// Antes esse loop so chamava lv_timer_handler() e ficava ~450-990ms
// sem processar o servidor web nenhuma vez - por isso o site "trava"/
// demora exatamente quando alguem passa o cartao. Agora ele mantem o
// HTTP respondendo durante a espera.
static void aguardarLoading(uint32_t ms)
{
    uint32_t inicio = millis();

    while (millis() - inicio < ms)
    {
        esp_task_wdt_reset();
        lv_timer_handler();
        processarServidorWeb();
        delay(5);
    }
}

void confirmarRemocaoRFID(int indice)
{
    aguardandoConfirmacao = false;
    if (indice < 0 ||
        indice >= quantidadeTrabalhadoresCadastrados)
        return;

    desativarTrabalhador(indice);
    registrarLog("SAIDA", indice);

    carregarTela(telaAtual);

    mostrarMensagemRFID(
        "REMOVIDO",
        cadastroTrabalhadores[indice].nome,
        "Trabalhador saiu da linha.",
        COR_PERIGO
    );
}

void cancelarRemocaoRFID()
{
    aguardandoConfirmacao = false;
    fecharConfirmacaoRFID();
}

void cadastrarTrabalhadorRFID(const char *uidTexto)
{
    int existente =
        encontrarTrabalhadorCadastrado(uidTexto);

    // Cartao ja cadastrado e ativo:
    // mostra loading e depois pede confirmacao.
    if (existente >= 0 &&
        cadastroTrabalhadores[existente].ativo)
    {
        mostrarLoadingRFID("LENDO CARTAO...");

        aguardarLoading(450);

        fecharLoadingRFID();

        aguardandoConfirmacao = true;

        if (!mostrarConfirmacaoRemocaoRFID(existente))
        {
            // O popup nao pode ser criado (memoria do LVGL
            // insuficiente no momento) - desiste dessa leitura em vez
            // de deixar o sistema preso esperando uma confirmacao que
            // nunca vai aparecer na tela.
            aguardandoConfirmacao = false;
        }


        return;
    }

    // Cartao ja conhecido, mas inativo:
    // volta a ativar.
    if (existente >= 0 &&
        !cadastroTrabalhadores[existente].ativo)
    {
        mostrarLoadingRFID("RECONHECENDO...");

        aguardarLoading(450);

        ativarTrabalhador(existente);
        registrarLog("ENTRADA", existente);

        fecharLoadingRFID();

        carregarTela(telaAtual);

        mostrarMensagemRFID(
            "REGISTRADO",
            cadastroTrabalhadores[existente].nome,
            "Trabalhador voltou para a linha.",
            COR_SUCESSO
        );

        return;
    }

    if (quantidadeTrabalhadoresCadastrados >=
        LIMITE_TRABALHADORES)
    {
        mostrarMensagemRFID(
            "LIMITE ATINGIDO",
            "",
            "Limite de cartoes cadastrado.",
            COR_PERIGO
        );

        return;
    }

    // Projeto de faculdade:
    // trabalhadores continuam ficticios, vinculados ao UID real do cartao.
    int idx =
        quantidadeTrabalhadoresCadastrados %
        QUANTIDADE_EXEMPLOS;

    Trabalhador *t =
        &cadastroTrabalhadores[
            quantidadeTrabalhadoresCadastrados
        ];

    mostrarLoadingRFID("REGISTRANDO CARTAO...");

    aguardarLoading(550);

    strncpy(
        t->nome,
        NOMES_EXEMPLO[idx],
        sizeof(t->nome) - 1
    );

    t->nome[sizeof(t->nome) - 1] = '\0';

    strncpy(
        t->funcao,
        FUNCOES_EXEMPLO[idx],
        sizeof(t->funcao) - 1
    );

    t->funcao[sizeof(t->funcao) - 1] = '\0';

    strncpy(
        t->uidCartao,
        uidTexto,
        sizeof(t->uidCartao) - 1
    );

    t->uidCartao[sizeof(t->uidCartao) - 1] = '\0';

    strncpy(
        t->empresa,
        "EMPRESA XYZ LTDA",
        sizeof(t->empresa) - 1
    );

    t->empresa[sizeof(t->empresa) - 1] = '\0';

    t->cadastrado = true;
    t->ativo = true;

    quantidadeTrabalhadoresCadastrados++;
    quantidadeTrabalhadoresAtivos++;

    atualizarEstadoBloqueio();

    registrarLog("ENTRADA", quantidadeTrabalhadoresCadastrados - 1);

    fecharLoadingRFID();

    carregarTela(telaAtual);

    mostrarMensagemRFID(
        "REGISTRADO",
        t->nome,
        "Cartao reconhecido com sucesso.",
        COR_SUCESSO
    );
}

// ============================================================
// LEITURA / DEBOUNCE
// ============================================================
//
// Antes o debounce era so por tempo (1500ms). Isso significa que um
// cartao deixado parado sobre o leitor era relido e reprocessado a
// cada 1500ms como se fosse um cadastro novo: cada leitura recria
// telas/overlays do LVGL e regrava o arquivo de logs na flash. Feito
// muitas vezes seguidas (varios cartoes em sequencia, ou um cartao
// esquecido em cima do leitor), isso fragmenta a memoria do LVGL e
// sobrecarrega o I2C do PN532 - e e a causa mais provavel do "trava
// se cadastrar varios ao mesmo tempo" e do travamento aleatorio depois
// de um tempo de uso.
//
// Agora so processamos de novo o MESMO cartao depois de um cooldown
// bem maior (ele precisa "descansar" fora do leitor por um tempo antes
// de contar como uma nova leitura). Um cartao DIFERENTE ainda pode ser
// lido no proximo ciclo de 1500ms normalmente.

static const uint32_t INTERVALO_POLL_MS = 1500;
static const uint32_t COOLDOWN_MESMO_CARTAO_MS = 4000;

static char ultimoUidProcessado[24] = "";
static uint32_t millisUltimoProcessamento = 0;

// Quantas falhas seguidas de leitura antes de tentar reinicializar o
// PN532. Uma colisao de 2 cartoes no campo, ou o modulo "engasgando",
// costuma deixar o leitor respondendo false/erro repetidamente ate ser
// reinicializado.
static const uint8_t FALHAS_PARA_REINICIAR_PN532 = 8;
static uint8_t falhasLeituraSeguidas = 0;

// ------------------------------------------------------------
// MEDICAO DO TEMPO DE RESPOSTA DO RFID
// ------------------------------------------------------------
//
// Mede o tempo entre a deteccao do cartao pelo PN532 e a conclusao do
// registro (cadastro/ativacao/log). Nao existe tela local para isso -
// os valores (ultima leitura, media e desvio padrao) ficam disponiveis
// no /estado.json, para nao gastar RAM/flash do ESP32 com uma
// interface extra. E so acompanhar o dashboard web enquanto se passa
// os cartoes no leitor.
//
// Media e desvio padrao sao calculados incrementalmente (algoritmo de
// Welford), sem guardar a lista de leituras - custa so 3 variaveis,
// independente de quantas leituras forem feitas.

uint32_t rfidQuantidadeLeituras = 0;
uint32_t rfidUltimoTempoRespostaMs = 0;
float rfidTempoRespostaMedioMs = 0.0f;
static float rfidSomaQuadradosDesvio = 0.0f;
float rfidTempoRespostaDesvioMs = 0.0f;

// Ultimas leituras (bancada de testes), so para preencher a tabela do
// TCC. Guarda so os ultimos 30 valores em RAM (60 bytes) - nao vai
// para o LittleFS nem fica persistido no backend, e o mais antigo e
// descartado quando o buffer enche (janela deslizante).
static const uint8_t RFID_ULTIMAS_LEITURAS_MAX = 30;
uint16_t rfidUltimasLeiturasMs[RFID_ULTIMAS_LEITURAS_MAX] = {};
uint8_t rfidUltimasLeiturasCount = 0;
static uint8_t rfidUltimasLeiturasPos = 0;

static void registrarTempoResposta(uint32_t tempoMs)
{
    rfidUltimoTempoRespostaMs = tempoMs;
    rfidQuantidadeLeituras++;

    uint32_t tempoLimitado = tempoMs > 65535 ? 65535 : tempoMs;
    rfidUltimasLeiturasMs[rfidUltimasLeiturasPos] = (uint16_t)tempoLimitado;
    rfidUltimasLeiturasPos = (rfidUltimasLeiturasPos + 1) % RFID_ULTIMAS_LEITURAS_MAX;

    if (rfidUltimasLeiturasCount < RFID_ULTIMAS_LEITURAS_MAX)
        rfidUltimasLeiturasCount++;

    // A soma acumulada (Welford) e feita em double so durante a conta,
    // pra nao perder precisao em somas repetidas - mas cada variavel
    // fica guardada como float (4 bytes), que sobra de precisao pra
    // medir milissegundos. Isso economiza 4 bytes de DRAM por variavel
    // em relacao a usar double o tempo todo (o ESP32 estava travando
    // no limite de RAM estatica).
    double delta = (double)tempoMs - (double)rfidTempoRespostaMedioMs;
    rfidTempoRespostaMedioMs += (float)(delta / (double)rfidQuantidadeLeituras);

    double delta2 = (double)tempoMs - (double)rfidTempoRespostaMedioMs;
    rfidSomaQuadradosDesvio += (float)(delta * delta2);

    if (rfidQuantidadeLeituras >= 2)
    {
        rfidTempoRespostaDesvioMs = (float)sqrt(
            (double)rfidSomaQuadradosDesvio / (double)(rfidQuantidadeLeituras - 1)
        );
    }

    Serial.print("RFID tempo de resposta (ms): ");
    Serial.print(tempoMs);
    Serial.print(" | media: ");
    Serial.print(rfidTempoRespostaMedioMs, 1);
    Serial.print(" | desvio padrao: ");
    Serial.print(rfidTempoRespostaDesvioMs, 1);
    Serial.print(" | n = ");
    Serial.println(rfidQuantidadeLeituras);
}

// Copia as leituras guardadas para "destino", da mais antiga para a
// mais nova (facilita numerar "Leitura 1, 2, 3..." como na tabela do
// TCC). Retorna quantas foram copiadas.
size_t copiarUltimasLeiturasRFID(uint16_t *destino, size_t max)
{
    size_t n = rfidUltimasLeiturasCount;
    if (n > max)
        n = max;

    // Se o buffer ja deu a volta, o mais antigo esta logo depois da
    // proxima posicao de escrita; senao, comeca do indice 0.
    uint8_t inicio = (rfidUltimasLeiturasCount < RFID_ULTIMAS_LEITURAS_MAX)
        ? 0
        : rfidUltimasLeiturasPos;

    for (size_t i = 0; i < n; i++)
    {
        destino[i] = rfidUltimasLeiturasMs[(inicio + i) % RFID_ULTIMAS_LEITURAS_MAX];
    }

    return n;
}

void lerRFID()
{
    if (!pn532Disponivel)
        return;

    if (aguardandoConfirmacao)
        return;


    static uint32_t ultimaLeitura = 0;


    if (
        millis() -
        ultimaLeitura <
        INTERVALO_POLL_MS
    )
    {
        return;
    }

    ultimaLeitura = millis();


    uint8_t uid[7];

    uint8_t uidLength = 0;


    bool sucesso =
        leitorNfc.readPassiveTargetID(
            PN532_MIFARE_ISO14443A,
            uid,
            &uidLength,
            100
        );


    if (!sucesso)
    {
        falhasLeituraSeguidas++;

        if (falhasLeituraSeguidas >= FALHAS_PARA_REINICIAR_PN532)
        {
            reiniciarPN532();
            falhasLeituraSeguidas = 0;
        }

        return;
    }

    falhasLeituraSeguidas = 0;

    // Marca o instante da deteccao - e daqui que o tempo de resposta
    // do teste de bancada e contado.
    uint32_t inicioMedicao = millis();


    char uidTexto[24];


    uidParaTexto(
        uid,
        uidLength,
        uidTexto,
        sizeof(uidTexto)
    );


    // Mesmo cartao ainda dentro do periodo de cooldown -> ignora.
    // Isso e o que evita reprocessar o cartao repetidas vezes enquanto
    // ele fica parado sobre o leitor. Uma leitura ignorada aqui nao
    // conta como medicao de tempo de resposta.
    bool mesmoCartao =
        strcmp(uidTexto, ultimoUidProcessado) == 0;

    if (
        mesmoCartao &&
        millis() - millisUltimoProcessamento < COOLDOWN_MESMO_CARTAO_MS
    )
    {
        return;
    }

    strncpy(
        ultimoUidProcessado,
        uidTexto,
        sizeof(ultimoUidProcessado) - 1
    );

    ultimoUidProcessado[sizeof(ultimoUidProcessado) - 1] = '\0';

    millisUltimoProcessamento = millis();


    cadastrarTrabalhadorRFID(
        uidTexto
    );

    registrarTempoResposta(
        millis() - inicioMedicao
    );
}