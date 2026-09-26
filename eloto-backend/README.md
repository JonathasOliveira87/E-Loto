# E-LOTO — backend

O ESP32 **manda** os dados (POST) para este servidor, que grava num SQLite.
O dashboard **lê** daqui. Ninguém abre conexão para dentro do ESP32.

```
ESP32 --POST /api/eventos--> backend --> eloto.db (SQLite) <--GET /api/painel-- dashboard
      --POST /api/estado -->
```

## Rodar

Precisa do Node.js 18 ou mais novo.

```bash
cd eloto-backend
npm install
ELOTO_KEY=minha-chave-secreta npm start        # Linux / macOS
```

No Windows (PowerShell):

```powershell
$env:ELOTO_KEY="minha-chave-secreta"; npm start
```

Variáveis (todas opcionais):

| variável   | padrão                | para quê                                  |
|------------|-----------------------|-------------------------------------------|
| `ELOTO_KEY`| `troque-esta-chave`   | chave que o ESP32 manda no `X-API-Key`    |
| `PORT`     | `3000`                | porta                                     |
| `ELOTO_DB` | `./eloto.db`          | onde fica o arquivo do banco              |
| `ELOTO_TZ` | `America/Sao_Paulo`   | fuso das datas mostradas no dashboard     |

## Dashboard (front)

O build do dashboard já vem na pasta `public/`, e o backend serve os dois juntos:

```
http://IP-DO-COMPUTADOR:3000   -> dashboard
http://IP-DO-COMPUTADOR:3000/api/...   -> API
```

Aberto por esse endereço, o dashboard descobre sozinho o servidor (não precisa
digitar nada). O código-fonte do front está em `eloto-front/`; depois de mexer
nele, `npm run build` e copie o conteúdo de `dist/` para `eloto-backend/public/`.

## Ligar o ESP32 nele

No `Config.h` do firmware:

```cpp
constexpr const char *SERVIDOR_URL   = "http://192.168.1.50:3000"; // IP DESTE computador
constexpr const char *SERVIDOR_CHAVE = "minha-chave-secreta";       // igual a ELOTO_KEY
```

Três cuidados que costumam dar problema:

1. **IP fixo neste computador.** Faça uma reserva de DHCP no roteador. Se o IP
   mudar, o ESP32 deixa de achar o servidor.
2. **Firewall.** No Windows, libere a porta 3000 de entrada (na primeira vez
   que o Node abrir a porta ele costuma perguntar; marque "Rede privada").
3. **Mesma rede.** ESP32 e computador precisam estar na mesma rede Wi-Fi/LAN.

## Testar sem o ESP32

```bash
curl http://localhost:3000/api/saude

curl -X POST http://localhost:3000/api/eventos \
  -H "Content-Type: application/json" -H "X-API-Key: minha-chave-secreta" \
  -d '{"origem_id":"teste-1","equipamento":"MESA PLANA","evento":"ENTRADA",
       "trabalhador":"FULANO","funcao":"MECANICO","uid":"04:AA:BB:CC","ts":0}'

curl http://localhost:3000/api/painel
```

## API

| rota                  | quem chama | o que faz                                              |
|-----------------------|------------|--------------------------------------------------------|
| `POST /api/eventos`   | ESP32      | grava `ENTRADA` / `SAIDA`. Repetir o mesmo `origem_id` é ignorado (o ESP reenvia se a resposta se perder). |
| `POST /api/estado`    | ESP32      | estado (BLOQUEADO/LIBERADO), diagnóstico. Enviado quando muda e a cada 30 s. |
| `GET /api/painel`     | dashboard  | `{ estado, logs }`. `logs` vem do mais novo para o mais antigo. `?limite=` (até 1000) e `?equipamento=`. |
| `GET /api/saude`      | qualquer   | `{ ok: true }`                                         |

`estado.esp_online` fica `false` se o ESP32 não fala com o servidor há mais de
90 s. `estado.ultimo_contato_s` diz há quantos segundos foi o último contato.

## Banco

Arquivo único `eloto.db`. Tabelas `eventos` (histórico) e `estado` (última
situação de cada equipamento). Para backup, copie o arquivo (com o servidor
parado, ou junto com `eloto.db-wal`).

Ver os dados direto:

```bash
sqlite3 eloto.db "SELECT datetime(ts,'unixepoch','-3 hours'), evento, trabalhador FROM eventos ORDER BY id DESC LIMIT 20;"
```

## Segurança

A chave protege contra qualquer aparelho da rede gravar eventos falsos, mas
trafega em HTTP puro dentro da LAN (dá para ler com um sniffer). Para uso fora
da rede local, ponha o backend atrás de HTTPS (por exemplo, proxy reverso).
O histórico não pode ser apagado pelo ESP32 nem pela API: é registro de
segurança.
