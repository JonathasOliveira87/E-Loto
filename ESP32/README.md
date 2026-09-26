# E-LOTO - Projeto modular para TCC

## Estrutura

- `ELOTO_Modular.ino` — ponto de entrada: setup() e loop().
- `Config.h` — constantes, pinos, dimensões, cores, limites e tipos principais.
- `Hardware.h/.cpp` — objetos físicos, display, PN532, buffers LVGL e funções de hardware.
- `Estado.h/.cpp` — estado global do sistema e cadastro dos trabalhadores.
- `RFID.h/.cpp` — inicialização do PN532, leitura do UID e lógica de cadastro/entrada/saída.
- `Touch.h/.cpp` — leitura do GT911 e screensaver.
- `UI.h/.cpp` — componentes visuais reutilizáveis: painel, botão, cadeado, cabeçalho e limpeza de tela.
- `Telas.h/.cpp` — navegação e construção das telas da aplicação.
- `data/screensaver.jpg` — imagem do descanso de tela, gravada no LittleFS (veja seção "CORRECAO DE FLASH" abaixo).
- `DEPENDENCIAS_EXTERNAS.txt` — arquivos/bibliotecas que precisam acompanhar o projeto original.


## Organização das variáveis

Os nomes principais foram deixados explícitos em português, por exemplo:
- `quantidadeTrabalhadoresAtivos`
- `quantidadeTrabalhadoresCadastrados`
- `equipamentoBloqueado`
- `pn532Disponivel`
- `ultimoToqueMillis`
- `PIN_RELE`
- `PIN_PN532_SDA`
- `PIN_PN532_SCL`

O campo que armazenava o UID RFID como `chapa` no código original passou a se chamar `uidCartao`, porque esse é o dado efetivamente armazenado.

## Comportamento preservado

A lógica principal foi mantida:
1. cartão novo: cadastra o trabalhador e o coloca como ativo;
2. cartão já cadastrado e ativo: retira o trabalhador da equipe ativa;
3. cartão já cadastrado e inativo: coloca novamente o trabalhador como ativo;
4. havendo trabalhador ativo, o equipamento fica bloqueado;
5. sem trabalhador ativo, o equipamento fica liberado.


## ALTERACOES DESTE TESTE

- O PN532 continua em I2C nos GPIO 21/22.
- O touch GT911 continua exatamente nos GPIO 33/32/21/25.
- Cartao novo recebe um trabalhador ficticio e fica associado ao UID.
- Ao aproximar novamente um cartao ativo aparece confirmacao:
  "DESEJA REMOVER? / Fulano / SIM / NAO".
- Entrada, saida e cadastro geram logs.
- Os eventos NAO ficam mais gravados no ESP32. Cada ENTRADA/SAIDA e
  enviada por POST para o backend (pasta eloto-backend), que grava no
  SQLite. O site le do banco.
- A tela HISTORICO mostra data, hora, evento, trabalhador e equipamento
  (ultimos registros, so em RAM).
- O botao APAGAR LOGS limpa so a lista da tela; o historico do servidor
  nao e apagado pelo equipamento.
- Configure em Config.h: SERVIDOR_URL (IP fixo do computador que roda o
  backend, ex.: http://192.168.1.50:3000) e SERVIDOR_CHAVE (igual a
  ELOTO_KEY do backend). O Wi-Fi da rede e cadastrado pela tela WIFI.
- O ESP32 nao tem mais servidor HTTP nem mDNS: so faz requisicoes para
  fora. Se o servidor estiver desligado, os eventos ficam numa fila em
  RAM (LIMITE_FILA_ENVIO) e sao reenviados quando ele voltar.

Os trabalhadores continuam ficticios de proposito. O UID do cartao real e
usado apenas como identificador do trabalhador ficticio.


## TESTE RAPIDO

1. Suba o backend (ver eloto-backend/README.md) e ajuste SERVIDOR_URL e
   SERVIDOR_CHAVE em Config.h.
2. Grave o projeto e cadastre o Wi-Fi pela tela WIFI.
3. Abra o Serial Monitor em 115200. Deve aparecer "[WIFI] conectado" e,
   a cada 30 s, "[DIAG] ...".
4. Aproxime o cartao branco: REGISTRADO + linha "[ENVIO] evento ENTRADA
   ... -> HTTP 200" no Serial.
5. Aproxime de novo, confirme SIM: gera SAIDA (tambem "-> HTTP 200").
6. Abra o dashboard: os eventos aparecem, vindos do banco.
7. Desligue o backend, passe o cartao, ligue o backend de novo: o evento
   sai da fila e aparece no dashboard.

CORRECAO DE MEMORIA ESP32:
LIMITE_LOGS foi reduzido para evitar overflow de DRAM do ESP32. Ele so alimenta a tela de historico local; o historico completo fica no SQLite do backend.


## CORRECAO DE FLASH (screensaver)

O arquivo `screensaver_image.h` original guardava a imagem do descanso
como um array de 76.800 posicoes (320x240 pixels RGB565) direto no
firmware. Isso ocupava sozinho cerca de 150KB de memoria flash do
ESP32, o que estourava o espaco disponivel em varias placas.

O que mudou:
- `screensaver_image.h` foi removido do firmware.
- A mesma imagem foi recomprimida como JPEG (qualidade 82) e agora
  ocupa cerca de 13KB. Ela fica em `data/screensaver.jpg`.
- O firmware le esse JPEG do LittleFS em tempo de execucao (biblioteca
  TJpg_Decoder), em vez de carregar tudo pronto na flash do programa.
- Se o arquivo nao estiver gravado no LittleFS, a tela de descanso
  aparece como uma tela preta solida, em vez de travar.

### Como enviar a imagem para o ESP32

A imagem em `data/screensaver.jpg` precisa ser gravada na particao
LittleFS do ESP32 (nao vai junto com o sketch automaticamente).
Duas formas de fazer isso:

**Arduino IDE 2.x:**
1. Instale a extensao "ESP32 LittleFS Data Upload" (arduino-littlefs-upload),
   disponivel no marketplace de extensoes do Arduino IDE 2.
2. Com a pasta do projeto aberta (contendo a pasta `data/`), use o
   comando da extensao (geralmente Ctrl+Shift+P -> "Upload LittleFS to
   Pico/ESP32...") para gravar o conteudo de `data/` no ESP32.

**Arduino IDE 1.8.x:**
1. Instale o plugin "ESP32 Sketch Data Upload" (arduino-esp32fs-plugin).
2. Va em Ferramentas -> "ESP32 Sketch Data Upload".

Depois de gravar os dados uma unica vez, o `/screensaver.jpg` fica
salvo na flash e nao precisa ser reenviado a cada nova gravacao do
sketch, a menos que voce queira trocar a imagem.

### Se quiser trocar a imagem no futuro

Basta substituir `data/screensaver.jpg` por outra imagem 320x240,
de preferencia ja em JPEG com qualidade entre 60-85 (arquivos entre
10-40KB ficam bem leves), e repetir o upload do LittleFS acima.


## OUTRA RECOMENDACAO IMPORTANTE (Partition Scheme)

Alem da reducao do screensaver, se o firmware ainda nao couber na
flash, va em Ferramentas -> "Partition Scheme" no Arduino IDE e
escolha um esquema sem OTA, como "Huge APP (3MB No OTA/1MB SPIFFS)"
ou "Minimal SPIFFS (1.9MB APP/190KB SPIFFS)". O esquema padrao reserva
metade da flash para uma segunda particao de OTA que este projeto nao
usa, entao trocar essa opcao libera bastante espaco sem mexer em
nenhuma linha de codigo.


## IMPORTANTE - imagem do screensaver
A imagem `data/screensaver.jpg` deve ser enviada para o LittleFS do ESP32.
Depois do upload do filesystem, ela fica disponível como `/screensaver.jpg`.
O firmware já usa `TJpg_Decoder` para desenhá-la diretamente no ST7789.
Se a imagem não for enviada ao LittleFS, o screensaver ficará preto por segurança.
