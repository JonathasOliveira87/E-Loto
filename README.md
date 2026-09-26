# E-LOTO

Sistema eletrônico de bloqueio e etiquetagem (LOTO — *Lockout/Tagout*) para
segurança industrial. Identifica trabalhadores por cartão RFID, controla o
bloqueio físico de um equipamento por um eletroímã e registra cada entrada e
saída num histórico de auditoria.

Projeto de TCC — Jonathas Robison de Oliveira.

## Como funciona

```
[ESP32 + RFID + display]  --HTTP POST-->  [Backend Node.js]  --HTTP GET-->  [Dashboard web]
      firmware                            Express + SQLite         React, servido pelo backend
```

- O **ESP32** lê os cartões, controla o relé/eletroímã e manda os eventos
  (entrada/saída) e o estado do equipamento para o backend, por HTTP POST.
  Se o backend estiver fora do ar, os eventos ficam numa fila local no ESP32
  e são reenviados quando a conexão voltar.
- O **backend** (Node.js + Express) recebe esses dados, grava o histórico
  num banco SQLite e serve o dashboard.
- O **dashboard** (React) mostra o status do equipamento, os trabalhadores
  ativos, o histórico de eventos e o diagnóstico do sistema em tempo real.
  Ele não fala com o ESP32 diretamente — só consome a API do backend.

## Estrutura do repositório

```
E-Loto/
├── ESP32/              firmware do microcontrolador (Arduino/C++)
├── eloto-backend/       servidor (Node.js + Express + SQLite)
│   └── public/          build do dashboard, servido junto com a API
└── eloto-frontend/      código-fonte do dashboard (React + Vite)
```

## Requisitos

- **Node.js** 18 ou mais novo — https://nodejs.org
- **Arduino IDE** (2.x) com suporte a placas ESP32 instalado
- Placa **ESP32** com display touch, leitor RFID **PN532**, módulo relé e
  eletroímã, conforme o hardware descrito no TCC
- Todos os aparelhos (computador do backend e ESP32) na **mesma rede Wi-Fi**

---

## 1. Rodando o site (backend + dashboard)

O dashboard já vem pronto dentro de `eloto-backend/public`, então basta subir
o backend — ele serve os dois juntos na mesma porta.

```bash
cd eloto-backend
npm install
```

No Windows (PowerShell):

```powershell
$env:ELOTO_KEY="sua-chave-aqui"; npm start
```

No Linux/macOS:

```bash
ELOTO_KEY=sua-chave-aqui npm start
```

Se tudo der certo, aparece:

```
E-LOTO backend na porta 3000
Banco: .../eloto-backend/eloto.db
```

Abra `http://localhost:3000` no navegador. De outro aparelho na mesma rede,
use o IP do computador, por exemplo `http://192.168.1.50:3000`.

### Variáveis de ambiente (todas opcionais)

| variável   | padrão                | para quê                                       |
|------------|------------------------|-------------------------------------------------|
| `ELOTO_KEY`| `troque-esta-chave`   | chave que o ESP32 usa para autenticar os envios |
| `PORT`     | `3000`                 | porta do servidor                               |
| `ELOTO_DB` | `./eloto.db`            | onde fica o arquivo do banco SQLite             |
| `ELOTO_TZ` | `America/Sao_Paulo`    | fuso horário usado nas datas do dashboard       |

**Importante:** use a mesma `ELOTO_KEY` aqui e no `SERVIDOR_CHAVE` do
firmware (seção 3), senão o ESP32 não consegue mandar dados.

### Deixando o backend sempre ligado (Windows)

Dentro de `eloto-backend` tem um `iniciar-eloto.bat`: dois cliques nele
instala as dependências (na primeira vez), liga o servidor com a chave
configurada e abre o dashboard no navegador.

### IP fixo

O IP do computador do backend pode mudar quando o roteador reatribui o
DHCP — e aí o ESP32 perde a conexão até você regravar o `Config.h`. Para
evitar isso, reserve um IP fixo para esse computador nas configurações do
roteador (procure por "Reserva de DHCP" ou "IP estático"), ou fixe o IP
direto no Windows (Configurações → Rede → Ethernet/Wi-Fi → Atribuição de
IP → Manual).

### Editando o dashboard

O código-fonte do site fica em `eloto-frontend/`. Depois de mexer:

```bash
cd eloto-frontend
npm install
npm run build
```

Copie o conteúdo de `eloto-frontend/dist/` para dentro de
`eloto-backend/public/`, substituindo o que já tinha lá. Não precisa
reiniciar o backend — só atualizar a página no navegador (Ctrl+F5).

---

## 2. Gravando o firmware no ESP32

1. Abra a Arduino IDE e instale o suporte à placa ESP32 (Boards Manager →
   procure "esp32", pacote da Espressif).
2. Instale as bibliotecas usadas pelo projeto (ver
   `ESP32/DEPENDENCIAS_EXTERNAS.txt` para a lista completa): LVGL,
   Arduino_GFX_Library, Adafruit PN532, entre outras.
3. Abra `ESP32/ELOTO_Modular.ino` na Arduino IDE — os outros arquivos da
   pasta (`.cpp`/`.h`) são carregados junto automaticamente.
4. Edite `ESP32/Config.h`:

   ```cpp
   // IP do computador que roda o backend, na mesma rede Wi-Fi.
   // Sem barra no final.
   constexpr const char *SERVIDOR_URL = "http://192.168.1.50:3000";

   // Precisa ser IGUAL ao ELOTO_KEY usado para ligar o backend.
   constexpr const char *SERVIDOR_CHAVE = "sua-chave-aqui";
   ```

5. Selecione a placa e a porta correspondentes (Ferramentas → Placa /
   Porta) e grave (Sketch → Carregar).
6. Ao ligar pela primeira vez, o equipamento abre a tela **WIFI**: informe
   o nome e a senha da rede por ali. A configuração fica salva na memória
   do ESP32 (LittleFS) mesmo depois de desligar.

### Conferindo que está tudo certo

Abra o **Serial Monitor** (115200 baud). Ao conectar no Wi-Fi, deve
aparecer:

```
[WIFI] conectado, IP=192.168.1.xx RSSI=-NN
```

Ao passar um cartão, deve aparecer:

```
[ENVIO] evento ENTRADA ... -> HTTP 200
```

`HTTP 200` confirma que o backend recebeu o evento. Um `HTTP -1` indica que
o ESP32 não alcançou o servidor (confira o IP e o firewall); `HTTP 401`
indica que a chave está diferente dos dois lados.

### Firewall (Windows)

Na primeira vez que o backend abre a porta, o Windows costuma perguntar se
permite o acesso — marque **Rede privada**. Sem isso, o ESP32 e outros
aparelhos da rede não conseguem alcançar o servidor.

---

## API do backend

| rota                 | quem chama | o que faz                                                    |
|-----------------------|------------|----------------------------------------------------------------|
| `POST /api/eventos`   | ESP32      | grava um evento de ENTRADA/SAÍDA                                |
| `POST /api/estado`    | ESP32      | atualiza status, diagnóstico e leituras de RFID da bancada      |
| `DELETE /api/eventos` | dashboard  | apaga o histórico do equipamento (exige a chave)                 |
| `GET /api/painel`     | dashboard  | devolve o estado atual e os últimos eventos                     |
| `GET /api/saude`      | qualquer   | checagem simples de que o servidor está no ar                    |

## Solução de problemas

- **O site abre mas fica "SEM DADOS"**: confira se o ESP32 está com o Wi-Fi
  conectado e se o `SERVIDOR_URL`/`SERVIDOR_CHAVE` no `Config.h` estão
  certos.
- **"Não foi possível falar com o servidor"**: confira se o backend está
  rodando, se o IP digitado é o do computador do backend (não o do ESP32),
  e se o firewall libera a porta.
- **O eletroímã não desbloqueia**: confira a fiação do relé e o diodo de
  proteção; veja o Serial Monitor para saber se o comando de bloqueio está
  chegando até o ESP32.

## Licença

Projeto acadêmico, desenvolvido como Trabalho de Conclusão de Curso.
