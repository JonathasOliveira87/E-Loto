// E-LOTO - backend
//
// O ESP32 manda um POST quando um trabalhador entra/sai (e um POST de
// estado quando o equipamento bloqueia/libera + a cada 30 s). Tudo vai
// para um SQLite. O dashboard le so daqui (GET /api/painel), nunca
// conversa direto com o ESP32.
//
//   POST /api/eventos   (ESP32)      grava ENTRADA / SAIDA
//   POST /api/estado    (ESP32)      atualiza estado + diagnostico
//   GET  /api/painel    (dashboard)  estado + ultimos eventos
//   DELETE /api/eventos (dashboard)  apaga o historico (exige a chave)
//   GET  /api/saude                  teste rapido "servidor no ar"

const path = require("path");
const fs = require("fs");
const crypto = require("crypto");
const express = require("express");
const cors = require("cors");
const Database = require("better-sqlite3");

const PORTA = Number(process.env.PORT) || 3000;
const CHAVE_API = process.env.ELOTO_KEY || "troque-esta-chave";
const ARQUIVO_DB =
  process.env.ELOTO_DB || path.join(__dirname, "eloto.db");
const FUSO = process.env.ELOTO_TZ || "America/Sao_Paulo";

// Se o ESP nao falar com o servidor por mais que isso, o painel mostra
// o controlador como offline. (O ESP manda estado a cada 30 s.)
const ESP_OFFLINE_APOS_S = 90;
const LIMITE_PADRAO_EVENTOS = 200;
const LIMITE_MAXIMO_EVENTOS = 1000;

// ------------------------------------------------------------------
// Banco
// ------------------------------------------------------------------

const db = new Database(ARQUIVO_DB);
db.pragma("journal_mode = WAL");

db.exec(`
  CREATE TABLE IF NOT EXISTS eventos (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    origem_id     TEXT UNIQUE,
    equipamento   TEXT NOT NULL,
    evento        TEXT NOT NULL,
    trabalhador   TEXT NOT NULL DEFAULT '',
    funcao        TEXT NOT NULL DEFAULT '',
    uid           TEXT NOT NULL DEFAULT '',
    ts            INTEGER NOT NULL,
    recebido_em   INTEGER NOT NULL
  );

  CREATE INDEX IF NOT EXISTS idx_eventos_equip_ts
    ON eventos (equipamento, ts DESC, id DESC);

  CREATE TABLE IF NOT EXISTS estado (
    equipamento           TEXT PRIMARY KEY,
    status                TEXT NOT NULL,
    trabalhadores_ativos  INTEGER NOT NULL DEFAULT 0,
    touchscreen           INTEGER NOT NULL DEFAULT 0,
    rfid_disponivel       INTEGER NOT NULL DEFAULT 0,
    rfid_leituras         INTEGER NOT NULL DEFAULT 0,
    rfid_ultimo_ms        INTEGER NOT NULL DEFAULT 0,
    rfid_medio_ms         REAL    NOT NULL DEFAULT 0,
    rfid_desvio_ms        REAL    NOT NULL DEFAULT 0,
    wifi_rssi             INTEGER NOT NULL DEFAULT 0,
    heap_livre            INTEGER NOT NULL DEFAULT 0,
    heap_min              INTEGER NOT NULL DEFAULT 0,
    uptime_s              INTEGER NOT NULL DEFAULT 0,
    wifi_reconexoes       INTEGER NOT NULL DEFAULT 0,
    atualizado_em         INTEGER NOT NULL
  );
`);

const inserirEvento = db.prepare(`
  INSERT OR IGNORE INTO eventos
    (origem_id, equipamento, evento, trabalhador, funcao, uid, ts, recebido_em)
  VALUES
    (@origem_id, @equipamento, @evento, @trabalhador, @funcao, @uid, @ts, @recebido_em)
`);

const salvarEstado = db.prepare(`
  INSERT INTO estado
    (equipamento, status, trabalhadores_ativos, touchscreen, rfid_disponivel,
     rfid_leituras, rfid_ultimo_ms, rfid_medio_ms, rfid_desvio_ms,
     wifi_rssi, heap_livre, heap_min, uptime_s, wifi_reconexoes, atualizado_em)
  VALUES
    (@equipamento, @status, @trabalhadores_ativos, @touchscreen, @rfid_disponivel,
     @rfid_leituras, @rfid_ultimo_ms, @rfid_medio_ms, @rfid_desvio_ms,
     @wifi_rssi, @heap_livre, @heap_min, @uptime_s, @wifi_reconexoes, @atualizado_em)
  ON CONFLICT(equipamento) DO UPDATE SET
    status               = excluded.status,
    trabalhadores_ativos = excluded.trabalhadores_ativos,
    touchscreen          = excluded.touchscreen,
    rfid_disponivel      = excluded.rfid_disponivel,
    rfid_leituras        = excluded.rfid_leituras,
    rfid_ultimo_ms       = excluded.rfid_ultimo_ms,
    rfid_medio_ms        = excluded.rfid_medio_ms,
    rfid_desvio_ms       = excluded.rfid_desvio_ms,
    wifi_rssi            = excluded.wifi_rssi,
    heap_livre           = excluded.heap_livre,
    heap_min             = excluded.heap_min,
    uptime_s             = excluded.uptime_s,
    wifi_reconexoes      = excluded.wifi_reconexoes,
    atualizado_em        = excluded.atualizado_em
`);

const apagarEventosDoEquipamento = db.prepare(
  "DELETE FROM eventos WHERE equipamento = ?"
);
const apagarTodosEventos = db.prepare("DELETE FROM eventos");

const buscarEstado = db.prepare(
  "SELECT * FROM estado WHERE equipamento = ?"
);
const buscarUltimoEstado = db.prepare(
  "SELECT * FROM estado ORDER BY atualizado_em DESC LIMIT 1"
);
const listarEventos = db.prepare(`
  SELECT id, evento, trabalhador, funcao, uid, equipamento, ts
    FROM eventos
   WHERE equipamento = ?
   ORDER BY ts DESC, id DESC
   LIMIT ?
`);
const contarEventos = db.prepare(
  "SELECT COUNT(*) AS total FROM eventos WHERE equipamento = ?"
);
const buscarUltimoEquipamento = db.prepare(
  "SELECT equipamento FROM eventos ORDER BY id DESC LIMIT 1"
);

// Ultimas leituras do RFID na bancada: só em memória (Map), nunca no
// SQLite. É só para preencher a tabela do TCC; reinicia o backend e
// esses valores se perdem (o histórico de eventos não é afetado).
const LIMITE_LEITURAS_BANCADA = 30;
const ultimasLeiturasPorEquipamento = new Map();

// ------------------------------------------------------------------
// Utilitarios
// ------------------------------------------------------------------

const agoraS = () => Math.floor(Date.now() / 1000);

function texto(valor, max) {
  if (valor === undefined || valor === null) return "";
  return String(valor).trim().slice(0, max);
}

function inteiro(valor, padrao = 0) {
  const n = Math.trunc(Number(valor));
  return Number.isFinite(n) ? n : padrao;
}

function decimal(valor, padrao = 0) {
  const n = Number(valor);
  return Number.isFinite(n) ? n : padrao;
}

function booleano(valor) {
  return valor === true || valor === 1 || valor === "true" ? 1 : 0;
}

// O ESP manda o horario dele (NTP) quando ja sincronizou; se veio 0,
// ou um valor absurdo, vale a hora do servidor.
function timestampValido(ts) {
  const n = inteiro(ts, 0);
  const agora = agoraS();
  if (n < 1_600_000_000) return agora;
  if (n > agora + 86_400) return agora;
  return n;
}

const fmtData = new Intl.DateTimeFormat("pt-BR", {
  timeZone: FUSO,
  day: "2-digit",
  month: "2-digit",
  year: "numeric",
});
const fmtHora = new Intl.DateTimeFormat("pt-BR", {
  timeZone: FUSO,
  hour: "2-digit",
  minute: "2-digit",
  second: "2-digit",
  hourCycle: "h23",
});

function comparaChave(recebida) {
  const a = Buffer.from(String(recebida || ""));
  const b = Buffer.from(CHAVE_API);
  if (a.length !== b.length) return false;
  return crypto.timingSafeEqual(a, b);
}

function exigirChave(req, res, next) {
  if (!comparaChave(req.get("x-api-key"))) {
    return res.status(401).json({ ok: false, erro: "chave invalida" });
  }
  next();
}

// ------------------------------------------------------------------
// App
// ------------------------------------------------------------------

const app = express();
app.use(cors());
app.use(express.json({ limit: "8kb" }));

app.get("/api/saude", (req, res) => {
  res.json({ ok: true, agora: agoraS() });
});

// ---- ESP32 -> servidor: evento de entrada / saida -----------------

app.post("/api/eventos", exigirChave, (req, res) => {
  const b = req.body || {};

  const evento = texto(b.evento, 16).toUpperCase();
  if (!/^[A-Z_]{1,16}$/.test(evento)) {
    return res.status(400).json({ ok: false, erro: "evento invalido" });
  }

  const equipamento = texto(b.equipamento, 40);
  if (!equipamento) {
    return res.status(400).json({ ok: false, erro: "equipamento ausente" });
  }

  const origemId = texto(b.origem_id, 40) || null;

  const info = inserirEvento.run({
    origem_id: origemId,
    equipamento,
    evento,
    trabalhador: texto(b.trabalhador, 60),
    funcao: texto(b.funcao, 60),
    uid: texto(b.uid, 40),
    ts: timestampValido(b.ts),
    recebido_em: agoraS(),
  });

  // changes === 0: o ESP reenviou um evento que ja tinha sido gravado
  // (a resposta anterior se perdeu). Responde 200 do mesmo jeito para
  // ele tirar o evento da fila.
  res.json({
    ok: true,
    duplicado: info.changes === 0,
    id: info.changes ? Number(info.lastInsertRowid) : null,
  });
});

// ---- ESP32 -> servidor: estado / heartbeat -------------------------

app.post("/api/estado", exigirChave, (req, res) => {
  const b = req.body || {};

  const equipamento = texto(b.equipamento, 40);
  if (!equipamento) {
    return res.status(400).json({ ok: false, erro: "equipamento ausente" });
  }

  const status = texto(b.status, 16).toUpperCase();
  if (status !== "BLOQUEADO" && status !== "LIBERADO") {
    return res.status(400).json({ ok: false, erro: "status invalido" });
  }

  // Array opcional com as últimas leituras de RFID (ms), só para a
  // bancada de testes. Guardado em memória, fora do SQLite.
  if (Array.isArray(b.rfid_ultimas_leituras_ms)) {
    const leituras = b.rfid_ultimas_leituras_ms
      .map((v) => inteiro(v, null))
      .filter((v) => v !== null && v >= 0 && v <= 65535)
      .slice(-LIMITE_LEITURAS_BANCADA);
    ultimasLeiturasPorEquipamento.set(equipamento, leituras);
  }

  salvarEstado.run({
    equipamento,
    status,
    trabalhadores_ativos: inteiro(b.trabalhadores_ativos),
    touchscreen: booleano(b.touchscreen),
    rfid_disponivel: booleano(b.rfid_disponivel),
    rfid_leituras: inteiro(b.rfid_leituras_medidas),
    rfid_ultimo_ms: inteiro(b.rfid_tempo_resposta_ultimo_ms),
    rfid_medio_ms: decimal(b.rfid_tempo_resposta_medio_ms),
    rfid_desvio_ms: decimal(b.rfid_tempo_resposta_desvio_ms),
    wifi_rssi: inteiro(b.wifi_rssi),
    heap_livre: inteiro(b.heap_livre),
    heap_min: inteiro(b.heap_min),
    uptime_s: inteiro(b.uptime_s),
    wifi_reconexoes: inteiro(b.wifi_reconexoes),
    atualizado_em: agoraS(),
  });

  res.json({ ok: true });
});

// ---- dashboard -> servidor: apagar historico ------------------------
// Protegido pela mesma chave do ESP32: o dashboard pede a chave na hora.

app.delete("/api/eventos", exigirChave, (req, res) => {
  const equipamento = texto(req.query.equipamento, 40);

  const info = equipamento
    ? apagarEventosDoEquipamento.run(equipamento)
    : apagarTodosEventos.run();

  console.log(
    `Historico apagado (${equipamento || "todos os equipamentos"}): ${info.changes} registro(s)`
  );

  res.json({ ok: true, apagados: info.changes });
});

// ---- dashboard <- servidor -----------------------------------------

app.get("/api/painel", (req, res) => {
  const limite = Math.min(
    Math.max(inteiro(req.query.limite, LIMITE_PADRAO_EVENTOS), 1),
    LIMITE_MAXIMO_EVENTOS
  );

  // Sem ?equipamento= usa o que mandou dado por ultimo.
  let equipamento = texto(req.query.equipamento, 40);
  if (!equipamento) {
    const e = buscarUltimoEstado.get();
    equipamento =
      (e && e.equipamento) ||
      (buscarUltimoEquipamento.get() || {}).equipamento ||
      "";
  }

  const e = equipamento ? buscarEstado.get(equipamento) : null;
  const agora = agoraS();
  const semContatoS = e ? agora - e.atualizado_em : null;

  const logs = equipamento
    ? listarEventos.all(equipamento, limite).map((l) => ({
        id: l.id,
        evento: l.evento,
        trabalhador: l.trabalhador,
        funcao: l.funcao,
        uid: l.uid,
        equipamento: l.equipamento,
        data: fmtData.format(new Date(l.ts * 1000)),
        hora: fmtHora.format(new Date(l.ts * 1000)),
        ts: l.ts,
      }))
    : [];

  const total = equipamento ? contarEventos.get(equipamento).total : 0;

  // Mesmos nomes de campo que o /estado.json do ESP tinha, para o
  // dashboard quase nao mudar. "esp_online" e novo.
  const estado = {
    sistema: "E-LOTO",
    equipamento: equipamento || "—",
    status: e ? e.status : "SEM DADOS",
    trabalhadores_ativos: e ? e.trabalhadores_ativos : 0,
    quantidade_logs: total,
    touchscreen: e ? !!e.touchscreen : false,
    rfid_disponivel: e ? !!e.rfid_disponivel : false,
    rfid_leituras_medidas: e ? e.rfid_leituras : 0,
    rfid_tempo_resposta_ultimo_ms: e ? e.rfid_ultimo_ms : 0,
    rfid_tempo_resposta_medio_ms: e ? e.rfid_medio_ms : 0,
    rfid_tempo_resposta_desvio_ms: e ? e.rfid_desvio_ms : 0,
    // So em memoria (nao vem do SQLite) - ver POST /api/estado.
    rfid_ultimas_leituras_ms: equipamento
      ? ultimasLeiturasPorEquipamento.get(equipamento) || []
      : [],
    wifi_rssi: e ? e.wifi_rssi : 0,
    heap_livre: e ? e.heap_livre : 0,
    heap_min: e ? e.heap_min : 0,
    uptime_s: e ? e.uptime_s : 0,
    wifi_reconexoes: e ? e.wifi_reconexoes : 0,
    esp_online: semContatoS !== null && semContatoS <= ESP_OFFLINE_APOS_S,
    ultimo_contato_s: semContatoS,
  };

  res.json({ estado, logs });
});

// Se existir a pasta ./public (build do dashboard), serve junto: o
// dashboard e a API ficam na mesma origem e nao precisa de CORS.
const pastaPublica = path.join(__dirname, "public");
if (fs.existsSync(pastaPublica)) {
  app.use(express.static(pastaPublica));
}

app.use((req, res) => {
  res.status(404).json({ ok: false, erro: "rota nao encontrada" });
});

// JSON invalido, corpo grande demais etc.
app.use((err, req, res, next) => {
  const status = err.status || 500;
  if (status >= 500) console.error(err);
  res.status(status).json({ ok: false, erro: "requisicao invalida" });
});

app.listen(PORTA, "0.0.0.0", () => {
  console.log(`E-LOTO backend na porta ${PORTA}`);
  console.log(`Banco: ${ARQUIVO_DB}`);
  if (CHAVE_API === "troque-esta-chave") {
    console.warn(
      "ATENCAO: usando a chave padrao. Defina ELOTO_KEY e a mesma chave no Config.h do ESP32."
    );
  }
});
