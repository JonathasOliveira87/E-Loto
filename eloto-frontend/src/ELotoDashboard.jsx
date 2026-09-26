import { useState, useEffect, useCallback, useRef } from "react";
import {
  Lock,
  Unlock,
  LogIn,
  LogOut,
  Users,
  ListOrdered,
  RefreshCw,
  Nfc,
  WifiOff,
  Wifi,
  AlertTriangle,
  Activity,
  Server,
  ShieldCheck,
  Clock3,
  Cable,
  Power,
  ExternalLink,
  Radio,
  Gauge,
  MonitorSmartphone,
} from "lucide-react";

const POLL_MS = 5000;
const FALHAS_PARA_OFFLINE = 3;
const FETCH_TIMEOUT_MS = 8000;

const CHAVE_STORAGE = "eloto_ip";

const ESTADO_PADRAO = {
  sistema: "E-LOTO",
  equipamento: "MESA PLANA",
  status: "SEM CONEXÃO",
  trabalhadores_ativos: 0,
  quantidade_logs: 0,
  touchscreen: false,
  rfid_disponivel: false,
  rfid_leituras_medidas: 0,
  rfid_tempo_resposta_ultimo_ms: 0,
  rfid_tempo_resposta_medio_ms: 0,
  rfid_tempo_resposta_desvio_ms: 0,
};

const IP_VALIDO_REGEX = /^[a-zA-Z0-9.-]+(:[0-9]{1,5})?$/;

// localStorage pode lançar erro (modo privado, política do navegador).
function lerEnderecoSalvo() {
  try {
    return localStorage.getItem(CHAVE_STORAGE) || "";
  } catch {
    return "";
  }
}

function salvarEndereco(valor) {
  try {
    localStorage.setItem(CHAVE_STORAGE, valor);
  } catch {
    /* sem persistência, segue a vida */
  }
}

function removerEndereco() {
  try {
    localStorage.removeItem(CHAVE_STORAGE);
  } catch {
    /* idem */
  }
}

export default function ELotoDashboard() {
  const [estado, setEstado] = useState(ESTADO_PADRAO);
  const [logs, setLogs] = useState([]);
  const [offline, setOffline] = useState(false);
  const [spinning, setSpinning] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [lastSync, setLastSync] = useState(null);
  const [erroIp, setErroIp] = useState("");
  const [activePage, setActivePage] = useState("painel");

  // "espIp" agora guarda o endereço do SERVIDOR (backend), ex.: 192.168.1.50:3000
  const [espIp, setEspIp] = useState(lerEnderecoSalvo);
  const [ipInput, setIpInput] = useState(lerEnderecoSalvo);

  const falhasSeguidasRef = useRef(0);
  const controllerRef = useRef(null);
  const carregandoRef = useRef(false);

  const mixedContent =
    typeof window !== "undefined" && window.location.protocol === "https:";

  const conectarESP = () => {
    const ip = ipInput
      .trim()
      .replace(/^https?:\/\//, "")
      .replace(/\/+$/, "");

    if (!ip) {
      setErroIp("Informe o endereço do servidor.");
      return;
    }

    if (!IP_VALIDO_REGEX.test(ip)) {
      setErroIp("Formato inválido. Ex.: 192.168.1.50:3000");
      return;
    }

    setErroIp("");
    falhasSeguidasRef.current = 0;
    salvarEndereco(ip);
    setIpInput(ip);
    setEspIp(ip);
    setConnecting(true);

    // O botão Conectar tem estado próprio.
    // A sincronização do topo continua independente.
    load(ip, true).finally(() => {
      setConnecting(false);
    });
  };

  const desconectarESP = () => {
    controllerRef.current?.abort();
    controllerRef.current = null;
    carregandoRef.current = false;
    removerEndereco();
    falhasSeguidasRef.current = 0;
    setEspIp("");
    setIpInput("");
    setErroIp("");
    setOffline(false);
    setLastSync(null);
    setEstado(ESTADO_PADRAO);
    setLogs([]);
  };

  const load = useCallback(
    async (ipForcado, manual = false) => {
      // Só aceita string: um onClick={load} antigo passava o EVENTO do
      // clique aqui e a URL virava "http://[object Object]".
      const alvo =
        typeof ipForcado === "string" && ipForcado ? ipForcado : espIp;

      if (!alvo) {
        setOffline(true);
        return;
      }

      // Já existe uma requisição em andamento: não abre outra por cima.
      // Só o clique manual (Conectar / Atualizar) interrompe a anterior.
      if (carregandoRef.current && !manual) return;

      controllerRef.current?.abort();

      const controller = new AbortController();
      controllerRef.current = controller;
      carregandoRef.current = true;

      const timeoutId = setTimeout(() => controller.abort(), FETCH_TIMEOUT_MS);

      setSpinning(true);

      try {
        const res = await fetch(
          `http://${alvo}/api/painel?t=${Date.now()}`,
          {
            signal: controller.signal,
            cache: "no-store",
          }
        );

        if (!res.ok) throw new Error(`Servidor respondeu ${res.status}`);

        const json = await res.json();

        falhasSeguidasRef.current = 0;
        setEstado(json.estado || ESTADO_PADRAO);
        setLogs(Array.isArray(json.logs) ? json.logs : []);
        setOffline(false);
        setLastSync(new Date());
      } catch (error) {
        // Esta requisição foi substituída por outra (ou o usuário desconectou).
        if (controllerRef.current !== controller) return;

        falhasSeguidasRef.current += 1;
        console.error("Erro ao falar com o servidor E-LOTO:", error);

        // Um clique manual merece feedback imediato; o polling em
        // segundo plano tolera algumas falhas antes de avisar.
        if (manual || falhasSeguidasRef.current >= FALHAS_PARA_OFFLINE) {
          setOffline(true);
        }
      } finally {
        clearTimeout(timeoutId);
        if (controllerRef.current === controller) {
          carregandoRef.current = false;
        }
        setTimeout(() => setSpinning(false), 350);
      }
    },
    [espIp]
  );

  // Se o site foi aberto pelo próprio backend (http://IP:3000), o servidor
  // é o mesmo endereço da página: descobre sozinho, sem digitar nada.
  useEffect(() => {
    if (espIp) return;

    let cancelado = false;

    fetch("/api/saude", { cache: "no-store" })
      .then((r) => (r.ok ? r.json() : null))
      .then((j) => {
        if (cancelado || !j || j.ok !== true) return;
        setIpInput(window.location.host);
        setEspIp(window.location.host);
      })
      .catch(() => {
        /* aberto de outro lugar (ex.: npm run dev): digitar o endereço */
      });

    return () => {
      cancelado = true;
    };
    // roda só uma vez, na abertura
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    if (!espIp) return;

    load();

    const timer = setInterval(() => {
      if (document.visibilityState === "visible") {
        load();
      }
    }, POLL_MS);

    const aoVoltarVisivel = () => {
      if (document.visibilityState === "visible") {
        load();
      }
    };

    document.addEventListener("visibilitychange", aoVoltarVisivel);

    // Não aborta a requisição aqui: ao clicar em Conectar o endereço muda
    // e este efeito roda de novo; abortar cancelaria a requisição manual.
    return () => {
      clearInterval(timer);
      document.removeEventListener("visibilitychange", aoVoltarVisivel);
    };
  }, [load, espIp]);

  // Ao fechar a página, cancela o que estiver em andamento.
  useEffect(() => {
    return () => controllerRef.current?.abort();
  }, []);

  const bloqueado = estado.status === "BLOQUEADO";
  const servidorOnline = Boolean(espIp) && !offline;
  // "online" = o ESP32 está de fato mandando dados para o servidor
  const online = servidorOnline && estado.esp_online !== false;

  const navegarMenu = (pagina) => {
    setActivePage(pagina);
    window.scrollTo({ top: 0, behavior: "smooth" });
  };

  return (
    <div className="eloto-app">
      <style>{`
        @import url('https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;500;600;700&family=Space+Grotesk:wght@500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap');

        * { box-sizing: border-box; }

        html, body, #root {
          margin: 0;
          min-height: 100%;
          background: #0a0e13;
        }

        body {
          font-family: "DM Sans", sans-serif;
        }

        button, input {
          font: inherit;
        }

        button {
          -webkit-tap-highlight-color: transparent;
        }

        .eloto-app {
          min-height: 100vh;
          color: #e8edf2;
          background:
            radial-gradient(circle at 85% 0%, rgba(245,183,0,.09), transparent 28%),
            radial-gradient(circle at 0% 100%, rgba(36,91,125,.12), transparent 32%),
            #0a0e13;
        }

        .eloto-shell {
          min-height: 100vh;
          display: grid;
          grid-template-columns: 240px minmax(0, 1fr);
        }

        .eloto-sidebar {
          position: sticky;
          top: 0;
          height: 100vh;
          padding: 22px 16px;
          border-right: 1px solid #1c2630;
          background: rgba(12,17,23,.94);
          backdrop-filter: blur(18px);
          display: flex;
          flex-direction: column;
          gap: 24px;
        }

        .brand {
          display: flex;
          align-items: center;
          gap: 12px;
          padding: 6px 8px;
        }

        .brand-mark {
          width: 38px;
          height: 38px;
          border-radius: 11px;
          display: grid;
          place-items: center;
          color: #101317;
          background: #f5b700;
          box-shadow: 0 8px 28px rgba(245,183,0,.18);
        }

        .brand-name {
          font: 700 17px "Space Grotesk", sans-serif;
          letter-spacing: -.02em;
        }

        .brand-sub {
          margin-top: 2px;
          color: #687582;
          font: 500 9px "JetBrains Mono", monospace;
          letter-spacing: .12em;
        }

        .nav-label {
          padding: 0 10px 8px;
          color: #56636f;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .14em;
        }

        .nav-item {
          width: 100%;
          border: 0;
          border-radius: 10px;
          padding: 11px 12px;
          display: flex;
          align-items: center;
          gap: 10px;
          color: #84909b;
          background: transparent;
          text-align: left;
          cursor: pointer;
        }

        .nav-item.active {
          color: #f1f4f6;
          background: #151d25;
          box-shadow: inset 3px 0 0 #f5b700;
        }

        .sidebar-bottom {
          margin-top: auto;
          padding: 12px;
          border: 1px solid #1d2933;
          border-radius: 13px;
          background: #10171e;
        }

        .sidebar-bottom-title {
          display: flex;
          align-items: center;
          gap: 7px;
          color: #a9b4bd;
          font-size: 11px;
          font-weight: 600;
        }

        .sidebar-bottom-value {
          margin-top: 8px;
          color: #f5b700;
          font: 600 11px "JetBrains Mono", monospace;
          word-break: break-all;
        }

        .main {
          width: 100%;
          max-width: 1320px;
          min-width: 0;
          margin: 0 auto;
          padding: 24px 34px 32px;
        }

        .topbar {
          min-height: 48px;
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 16px;
          margin-bottom: 22px;
        }

        .page-kicker {
          color: #62707c;
          font: 500 10px "JetBrains Mono", monospace;
          letter-spacing: .12em;
        }

        .page-title {
          margin: 4px 0 0;
          font: 700 25px "Space Grotesk", sans-serif;
          letter-spacing: -.04em;
        }

        .top-actions {
          display: flex;
          align-items: center;
          gap: 9px;
        }

        .sync-chip {
          min-height: 38px;
          padding: 0 12px;
          display: inline-flex;
          align-items: center;
          justify-content: center;
          gap: 8px;
          line-height: 1;
          border: 1px solid #1e2a34;
          border-radius: 10px;
          background: #10171e;
          color: #7e8b96;
          font: 500 10px "JetBrains Mono", monospace;
        }

        .sync-dot {
          width: 7px;
          min-width: 7px;
          height: 7px;
          min-height: 7px;
          flex: 0 0 7px;
          display: block;
          align-self: center;
          border-radius: 50%;
          background: #35b37e;
          box-shadow: 0 0 0 4px rgba(53,179,126,.08);
        }

        .sync-dot-offline {
          width: 7px;
          min-width: 7px;
          height: 7px;
          min-height: 7px;
          flex: 0 0 7px;
          background: #e4483c;
          box-shadow: none;
        }

        .refresh {
          width: 38px;
          height: 38px;
          display: grid;
          place-items: center;
          border: 1px solid #25313c;
          border-radius: 10px;
          background: #10171e;
          color: #a7b1b9;
          cursor: pointer;
        }

        .refresh:hover {
          border-color: #3b4a56;
          color: #f5b700;
        }

        .refresh.spinning svg {
          animation: spin .65s linear infinite;
        }

        @keyframes spin {
          to { transform: rotate(360deg); }
        }

        .content-grid {
          display: grid;
          grid-template-columns: minmax(0, 1.25fr) minmax(340px, .75fr);
          gap: 18px;
          align-items: start;
        }

        .card {
          scroll-margin-top: 24px;
          border: 1px solid #1d2832;
          border-radius: 16px;
          background: linear-gradient(145deg, #111820, #0e141a);
          box-shadow: 0 18px 55px rgba(0,0,0,.18);
        }

        .hero {
          position: relative;
          min-height: 270px;
          padding: 22px;
          overflow: hidden;
        }

        .hero:after {
          content: "";
          position: absolute;
          width: 260px;
          height: 260px;
          right: -110px;
          top: -120px;
          border: 1px solid rgba(245,183,0,.10);
          border-radius: 50%;
          box-shadow:
            0 0 0 35px rgba(245,183,0,.025),
            0 0 0 70px rgba(245,183,0,.018);
        }

        .hero-head {
          display: flex;
          justify-content: space-between;
          align-items: flex-start;
          gap: 15px;
        }

        .equipment-label {
          color: #64717d;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .15em;
        }

        .equipment-name {
          margin: 7px 0 0;
          font: 700 30px "Space Grotesk", sans-serif;
          letter-spacing: -.045em;
        }

        .system-tag {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          margin-top: 8px;
          color: #f5b700;
          font: 600 10px "JetBrains Mono", monospace;
        }

        .state-badge {
          flex-shrink: 0;
          display: inline-flex;
          align-items: center;
          gap: 7px;
          padding: 7px 9px;
          border-radius: 9px;
          border: 1px solid;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .07em;
        }

        .state-badge.online {
          color: #35b37e;
          background: rgba(53,179,126,.06);
          border-color: rgba(53,179,126,.22);
        }

        .state-badge.offline {
          color: #e4483c;
          background: rgba(228,72,60,.06);
          border-color: rgba(228,72,60,.22);
        }

        .state-center {
          display: flex;
          align-items: center;
          gap: 18px;
          margin-top: 34px;
        }

        .state-icon {
          width: 78px;
          height: 78px;
          flex-shrink: 0;
          display: grid;
          place-items: center;
          border-radius: 22px;
          border: 1px solid;
        }

        .state-icon.unlocked {
          color: #35b37e;
          background: rgba(53,179,126,.07);
          border-color: rgba(53,179,126,.22);
        }

        .state-icon.locked {
          color: #e4483c;
          background: rgba(228,72,60,.07);
          border-color: rgba(228,72,60,.22);
          animation: lockedPulse 1.8s ease-in-out infinite;
        }

        @keyframes lockedPulse {
          50% { box-shadow: 0 0 0 8px rgba(228,72,60,.035); }
        }

        .state-overline {
          color: #63707b;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .13em;
        }

        .state-value {
          margin-top: 5px;
          font: 700 28px "Space Grotesk", sans-serif;
          letter-spacing: -.035em;
        }

        .state-description {
          margin-top: 5px;
          color: #6f7b86;
          font-size: 11px;
        }

        .metrics {
          display: grid;
          grid-template-columns: repeat(2, 1fr);
          gap: 12px;
          margin-top: 18px;
        }

        .metric {
          padding: 16px;
          border: 1px solid #1d2832;
          border-radius: 14px;
          background: #0d141a;
        }

        .metric-head {
          display: flex;
          align-items: center;
          justify-content: space-between;
          color: #687581;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .09em;
        }

        .metric-icon {
          color: #f5b700;
          opacity: .9;
        }

        .metric-number {
          margin-top: 13px;
          font: 700 28px "Space Grotesk", sans-serif;
        }

        .metric-note {
          margin-top: 3px;
          color: #596773;
          font-size: 10px;
        }

        .diag-card {
          padding: 17px;
        }

        .diag-status-list {
          display: grid;
          gap: 9px;
        }

        .diag-status-item {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 10px;
          padding: 10px 12px;
          border: 1px solid #1d2832;
          border-radius: 10px;
          background: #0d141a;
        }

        .diag-status-left {
          display: flex;
          align-items: center;
          gap: 9px;
          color: #c3cbd2;
          font-size: 11.5px;
          font-weight: 600;
        }

        .diag-status-left svg {
          color: #687581;
        }

        .diag-status-tag {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          color: #35b37e;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .06em;
        }

        .diag-status-tag.off {
          color: #e4483c;
        }

        .diag-measure-grid {
          margin-top: 12px;
          display: grid;
          grid-template-columns: repeat(4, 1fr);
          gap: 7px;
        }

        .diag-measure-cell {
          padding: 9px 4px;
          text-align: center;
          border: 1px solid #1d2832;
          border-radius: 9px;
          background: #0d141a;
        }

        .diag-measure-num {
          font: 700 16px "Space Grotesk", sans-serif;
          color: #f5b700;
        }

        .diag-measure-lbl {
          margin-top: 3px;
          color: #5e6b76;
          font: 600 7.5px "JetBrains Mono", monospace;
          letter-spacing: .05em;
        }

        .diag-measure-hint {
          margin-top: 10px;
          color: #596773;
          font-size: 9.5px;
          line-height: 1.5;
        }

        .connection {
          padding: 18px;
        }

        .card-title-row {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 10px;
          margin-bottom: 15px;
        }

        .card-title {
          display: flex;
          align-items: center;
          gap: 8px;
          color: #dfe5e9;
          font: 600 12px "Space Grotesk", sans-serif;
        }

        .card-title svg {
          color: #f5b700;
        }

        .card-mini {
          color: #53606b;
          font: 500 9px "JetBrains Mono", monospace;
        }

        .input-wrap {
          display: flex;
          gap: 8px;
        }

        .ip-input {
          min-width: 0;
          flex: 1;
          height: 42px;
          padding: 0 12px;
          color: #e6ebee;
          background: #090e13;
          border: 1px solid #26323d;
          border-radius: 10px;
          outline: none;
          font: 500 11px "JetBrains Mono", monospace;
        }

        .ip-input:focus {
          border-color: #f5b700;
          box-shadow: 0 0 0 3px rgba(245,183,0,.08);
        }

        .connect {
          height: 42px;
          padding: 0 15px;
          border: 0;
          border-radius: 10px;
          color: #101317;
          background: #f5b700;
          font: 700 10px "JetBrains Mono", monospace;
          cursor: pointer;
        }

        .connect:hover {
          background: #ffc51c;
        }

        .connect:disabled {
          opacity: .65;
          cursor: default;
          background: #c99a1f;
        }

        .error {
          margin-top: 8px;
          color: #e4483c;
          font: 500 10px "JetBrains Mono", monospace;
        }

        .connection-details {
          margin-top: 13px;
          padding-top: 13px;
          border-top: 1px solid #1b2630;
          display: grid;
          gap: 7px;
        }

        .detail-row {
          display: flex;
          justify-content: space-between;
          gap: 15px;
          color: #697681;
          font-size: 10px;
        }

        .detail-row strong {
          color: #cbd3d8;
          font-family: "JetBrains Mono", monospace;
          font-weight: 500;
        }

        .detail-url {
          color: #35b37e !important;
          word-break: break-all;
        }

        .disconnect {
          margin-top: 5px;
          justify-self: start;
          padding: 7px 9px;
          border: 1px solid rgba(228,72,60,.22);
          border-radius: 8px;
          background: rgba(228,72,60,.05);
          color: #e4483c;
          font: 600 9px "JetBrains Mono", monospace;
          cursor: pointer;
        }

        .warning {
          margin-top: 12px;
          padding: 10px 11px;
          display: flex;
          align-items: flex-start;
          gap: 8px;
          color: #f5b700;
          border: 1px solid rgba(245,183,0,.2);
          border-radius: 10px;
          background: rgba(245,183,0,.045);
          font: 500 10px/1.5 "JetBrains Mono", monospace;
        }

        .offline {
          margin-top: 12px;
          padding: 10px 11px;
          display: flex;
          align-items: center;
          gap: 8px;
          color: #e4483c;
          border: 1px solid rgba(228,72,60,.2);
          border-radius: 10px;
          background: rgba(228,72,60,.045);
          font: 500 10px "JetBrains Mono", monospace;
        }

        .logs-card {
          grid-column: 2;
          grid-row: 1 / span 4;
          min-width: 0;
          padding: 18px;
          position: sticky;
          top: 24px;
          align-self: start;
          max-height: calc(100vh - 48px);
          display: flex;
          flex-direction: column;
        }

        /* Painel principal permanece na coluna esquerda;
           Atividade recente fica acompanhando o usuário na coluna direita. */
        .content-grid > .hero,
        .content-grid > .connection,
        .content-grid > .metrics,
        .content-grid > .diag-card {
          grid-column: 1;
        }

        .logs-header {
          display: flex;
          align-items: center;
          justify-content: space-between;
          gap: 15px;
          margin-bottom: 12px;
        }

        .logs-title {
          display: flex;
          align-items: center;
          gap: 9px;
          font: 600 13px "Space Grotesk", sans-serif;
        }

        .logs-title svg {
          color: #f5b700;
        }

        .log-count {
          padding: 6px 8px;
          border: 1px solid #202c36;
          border-radius: 7px;
          color: #6d7a85;
          font: 500 9px "JetBrains Mono", monospace;
        }

        .logs-list {
          max-height: calc(100vh - 145px);
          min-height: 120px;
          overflow-y: auto;
          display: grid;
          gap: 7px;
          padding-right: 3px;
        }

        .logs-list::-webkit-scrollbar {
          width: 6px;
        }

        .logs-list::-webkit-scrollbar-track {
          background: #0b1015;
        }

        .logs-list::-webkit-scrollbar-thumb {
          background: #26333e;
          border-radius: 8px;
        }

        .log-row {
          display: grid;
          grid-template-columns: 38px minmax(0, 1fr) auto;
          align-items: center;
          gap: 12px;
          padding: 12px;
          border: 1px solid #1b2731;
          border-radius: 11px;
          background: #0d141a;
          transition: .15s ease;
        }

        .log-row:hover {
          border-color: #2a3945;
          transform: translateY(-1px);
        }

        .event-icon {
          width: 34px;
          height: 34px;
          display: grid;
          place-items: center;
          border-radius: 9px;
        }

        .event-icon.entrada {
          color: #f5b700;
          background: rgba(245,183,0,.08);
        }

        .event-icon.saida {
          color: #e4483c;
          background: rgba(228,72,60,.08);
        }

        .worker {
          color: #e3e8eb;
          font-size: 12px;
          font-weight: 600;
        }

        .function {
          margin-top: 2px;
          color: #64717c;
          font-size: 10px;
        }

        .meta {
          margin-top: 5px;
          display: flex;
          flex-wrap: wrap;
          gap: 8px;
          color: #4f5d68;
          font: 500 9px "JetBrains Mono", monospace;
        }

        .uid {
          display: inline-flex;
          align-items: center;
          gap: 4px;
          color: #788590;
        }

        .event {
          align-self: center;
          padding: 5px 7px;
          border: 1px solid;
          border-radius: 6px;
          font: 600 8px "JetBrains Mono", monospace;
          letter-spacing: .06em;
        }

        .event.entrada {
          color: #f5b700;
          border-color: rgba(245,183,0,.2);
        }

        .event.saida {
          color: #e4483c;
          border-color: rgba(228,72,60,.2);
        }

        .empty {
          padding: 35px 10px;
          text-align: center;
          color: #586671;
          font-size: 12px;
        }

        .equipment-products {
          display: grid;
          grid-template-columns: repeat(2, minmax(0, 1fr));
          gap: 18px;
        }

        .equipment-product-card {
          min-width: 0;
          overflow: hidden;
          border: 1px solid #202a34;
          border-radius: 18px;
          background: linear-gradient(180deg, #111820 0%, #0e141b 100%);
          box-shadow: 0 18px 45px rgba(0,0,0,.18);
        }

        .product-image-wrap {
          position: relative;
          height: 220px;
          display: flex;
          align-items: center;
          justify-content: center;
          overflow: hidden;
          background: #f1f3f5;
          border-bottom: 1px solid #202a34;
        }

        .product-image {
          width: 100%;
          height: 100%;
          object-fit: contain;
          padding: 14px;
        }

        .product-image-solenoid { padding: 24px; }

        .product-live {
          position: absolute;
          top: 12px;
          right: 12px;
          display: inline-flex;
          align-items: center;
          gap: 7px;
          padding: 7px 10px;
          border: 1px solid rgba(53,179,126,.3);
          border-radius: 999px;
          background: rgba(7,14,19,.9);
          color: #9be6c5;
          font: 700 9px "JetBrains Mono", monospace;
          letter-spacing: .08em;
        }

        .product-live.off { border-color: #3a444f; color: #a8b1ba; }
        .status-dot { width: 6px; height: 6px; border-radius: 50%; background: #35b37e; display: inline-block; }
        .product-live.off .status-dot { background: #6d7782; }

        .product-body { padding: 19px; }
        .product-category { color: #8d9aa7; font: 700 9px "JetBrains Mono", monospace; letter-spacing: .12em; margin-bottom: 8px; }
        .product-body h3 { margin: 0; color: #f2f5f7; font: 700 20px "Space Grotesk", sans-serif; line-height: 1.15; }
        .product-description { margin: 10px 0 16px; color: #87929e; font-size: 12px; line-height: 1.55; }

        .spec-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
        .spec-grid > div { padding: 9px; border: 1px solid #202a34; border-radius: 10px; background: #0b1117; min-width: 0; }
        .spec-grid span { display: block; color: #687582; font: 700 8px "JetBrains Mono", monospace; text-transform: uppercase; margin-bottom: 4px; }
        .spec-grid strong { display: block; color: #dce3e9; font-size: 10px; line-height: 1.3; word-break: break-word; }

        .product-links { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 15px; }
        .product-link { display: inline-flex; align-items: center; gap: 6px; padding: 8px 10px; border: 1px solid #29343f; border-radius: 9px; color: #b8c3cd; background: #111820; text-decoration: none; font: 700 9px "JetBrains Mono", monospace; transition: .2s ease; }
        .product-link:hover { border-color: #51606e; transform: translateY(-1px); }
        .product-link.primary { border-color: rgba(245,183,0,.35); color: #f5c94d; background: rgba(245,183,0,.06); }
        .product-note { margin-top: 12px; color: #707d89; font-size: 10px; line-height: 1.45; }
        .equipment-detail-title { grid-column: 1 / -1; display: flex; align-items: center; gap: 8px; color: #c5ced6; font: 700 10px "JetBrains Mono", monospace; letter-spacing: .08em; margin-bottom: 2px; }

        @media (max-width: 1120px) {
          .equipment-products { grid-template-columns: 1fr; }
          .product-image-wrap { height: 260px; }
        }

        .footer {
          margin-top: 18px;
          display: flex;
          justify-content: space-between;
          gap: 12px;
          color: #43515c;
          font: 500 9px "JetBrains Mono", monospace;
        }

        .equipment-page {
          display: grid;
          gap: 18px;
          animation: pageIn .28s ease both;
        }

        @keyframes pageIn {
          from { opacity: 0; transform: translateY(7px); }
          to { opacity: 1; transform: translateY(0); }
        }

        .equipment-intro {
          padding: 22px;
        }

        .equipment-intro h2 {
          margin: 5px 0 7px;
          font: 700 24px "Space Grotesk", sans-serif;
        }

        .equipment-intro p {
          margin: 0;
          color: #74818c;
          font-size: 13px;
        }

        .equipment-list {
          display: grid;
          grid-template-columns: repeat(2, minmax(0, 1fr));
          gap: 14px;
        }

        .equipment-item {
          padding: 19px;
          display: flex;
          align-items: center;
          gap: 14px;
          min-width: 0;
          border: 1px solid #1d2832;
          border-radius: 15px;
          background: linear-gradient(145deg, #111820, #0e141a);
          box-shadow: 0 15px 45px rgba(0,0,0,.14);
        }

        .equipment-item-icon {
          width: 48px;
          height: 48px;
          flex: 0 0 48px;
          display: grid;
          place-items: center;
          border-radius: 13px;
          background: #151e27;
          color: #f5b700;
          border: 1px solid #26333e;
        }

        .equipment-item-main {
          min-width: 0;
          flex: 1;
        }

        .equipment-item-name {
          color: #edf1f4;
          font: 700 15px "Space Grotesk", sans-serif;
        }

        .equipment-item-type {
          margin-top: 4px;
          color: #687580;
          font-size: 11px;
        }

        .equipment-item-status {
          display: inline-flex;
          align-items: center;
          gap: 6px;
          margin-top: 9px;
          color: #35b37e;
          font: 600 9px "JetBrains Mono", monospace;
          letter-spacing: .08em;
        }

        .equipment-item-status.off {
          color: #e4483c;
        }

        .status-dot {
          width: 7px;
          height: 7px;
          border-radius: 50%;
          background: currentColor;
        }

        .equipment-detail {
          padding: 20px;
          display: grid;
          grid-template-columns: repeat(2, minmax(0, 1fr));
          gap: 10px;
          align-items: stretch;
        }

        .equipment-detail-row {
          padding: 13px 14px;
          border: 1px solid #1d2933;
          border-radius: 11px;
          background: #0d141a;
        }

        .equipment-detail-row span {
          display: block;
          color: #687580;
          font: 500 9px "JetBrains Mono", monospace;
          letter-spacing: .08em;
        }

        .equipment-detail-row strong {
          display: block;
          margin-top: 6px;
          color: #dfe5e9;
          font-size: 13px;
          word-break: break-word;
        }

        @media (max-width: 1050px) {
          .eloto-shell {
            grid-template-columns: 76px minmax(0, 1fr);
          }

          .eloto-sidebar {
            padding: 18px 10px;
          }

          .brand {
            justify-content: center;
            padding: 0;
          }

          .brand-text,
          .nav-label,
          .nav-item span,
          .sidebar-bottom {
            display: none;
          }

          .nav-item {
            justify-content: center;
            padding: 12px;
          }

          .content-grid {
            grid-template-columns: 1fr;
          }

          .equipment-list {
            grid-template-columns: 1fr;
          }

          .logs-card {
            grid-column: auto;
            grid-row: auto;
            position: static;
            max-height: none;
          }

          .content-grid > .hero,
          .content-grid > .connection,
          .content-grid > .metrics,
          .content-grid > .diag-card {
            grid-column: auto;
          }
        }

        @media (max-width: 700px) {
          .eloto-shell {
            display: block;
          }

          .eloto-sidebar {
            position: static;
            width: 100%;
            height: auto;
            padding: 12px 14px;
            border-right: 0;
            border-bottom: 1px solid #1c2630;
            flex-direction: row;
            align-items: center;
          }

          .brand {
            justify-content: flex-start;
          }

          .brand-text {
            display: block;
          }

          .nav-label,
          .sidebar-bottom {
            display: none;
          }

          .eloto-nav {
            margin-left: auto;
            display: flex;
            gap: 3px;
          }

          .nav-item {
            width: 40px;
            justify-content: center;
          }

          .nav-item span {
            display: none;
          }

          .main {
            max-width: none;
            padding: 18px 14px 25px;
          }

          .topbar {
            align-items: flex-start;
          }

          .sync-chip {
            display: none;
          }

          .hero {
            min-height: auto;
          }

          .equipment-detail {
            grid-template-columns: 1fr;
          }

          .equipment-name {
            font-size: 25px;
          }

          .state-center {
            margin-top: 25px;
          }

          .log-row {
            grid-template-columns: 34px minmax(0, 1fr);
          }

          .event {
            display: none;
          }

          .footer {
            flex-direction: column;
          }
        }

        @media (max-width: 480px) {
          .input-wrap {
            flex-direction: column;
          }

          .connect {
            width: 100%;
          }

          .state-icon {
            width: 62px;
            height: 62px;
            border-radius: 17px;
          }

          .state-value {
            font-size: 22px;
          }

          .metrics {
            gap: 8px;
          }

          .metric {
            padding: 13px;
          }

          .diag-measure-grid {
            grid-template-columns: repeat(2, 1fr);
          }
        }

        @media (prefers-reduced-motion: reduce) {
          .refresh.spinning svg,
          .state-icon.locked {
            animation: none;
          }
        }
      `}</style>

      <div className="eloto-shell">
        <aside className="eloto-sidebar">
          <div className="brand">
            <div className="brand-mark">
              <ShieldCheck size={21} strokeWidth={2.5} />
            </div>
            <div className="brand-text">
              <div className="brand-name">E-LOTO</div>
              <div className="brand-sub">CONTROL CENTER</div>
            </div>
          </div>

          <nav className="eloto-nav">
            <div className="nav-label">MONITORAMENTO</div>
            <button
              className={`nav-item ${activePage === "painel" ? "active" : ""}`}
              onClick={() => navegarMenu("painel")}
            >
              <Activity size={17} />
              <span>Painel geral</span>
            </button>
            <button
              className={`nav-item ${activePage === "equipamentos" ? "active" : ""}`}
              onClick={() => navegarMenu("equipamentos")}
            >
              <Server size={17} />
              <span>Equipamentos</span>
            </button>
          </nav>

          <div className="sidebar-bottom">
            <div className="sidebar-bottom-title">
              <Cable size={14} />
              Servidor conectado
            </div>
            <div className="sidebar-bottom-value">
              {espIp || "não configurado"}
            </div>
          </div>
        </aside>

        <main className="main" id="painel">
          <header className="topbar">
            <div>
              <div className="page-kicker">SISTEMA DE SEGURANÇA INDUSTRIAL</div>
              <h1 className="page-title">{activePage === "equipamentos" ? "Equipamentos" : "Visão geral"}</h1>
            </div>

            <div className="top-actions">
              <div className="sync-chip">
                <span className={`sync-dot ${servidorOnline ? "" : "sync-dot-offline"}`} />
                {lastSync
                  ? `SINCRONIZADO ${lastSync.toLocaleTimeString("pt-BR")}`
                  : "AGUARDANDO DADOS"}
              </div>

              <button
                className={`refresh ${spinning ? "spinning" : ""}`}
                onClick={() => load(undefined, true)}
                aria-label="Atualizar dados"
              >
                <RefreshCw size={17} />
              </button>
            </div>
          </header>

          {activePage === "painel" ? (
            <section className="content-grid">
              <div className="card hero" id="equipamento">
                <div className="hero-head">
                  <div>
                    <div className="equipment-label">EQUIPAMENTO MONITORADO</div>
                    <h2 className="equipment-name">
                      {estado.equipamento || "MESA PLANA"}
                    </h2>
                    <div className="system-tag">
                      <Power size={12} />
                      {estado.sistema || "E-LOTO"}
                    </div>
                  </div>

                  <div className={`state-badge ${online ? "online" : "offline"}`}>
                    {online ? <Wifi size={12} /> : <WifiOff size={12} />}
                    {online ? "ONLINE" : "OFFLINE"}
                  </div>
                </div>

                <div className="state-center">
                  <div
                    className={`state-icon ${bloqueado ? "locked" : "unlocked"
                      }`}
                  >
                    {bloqueado ? (
                      <Lock size={32} />
                    ) : (
                      <Unlock size={32} />
                    )}
                  </div>

                  <div>
                    <div className="state-overline">STATUS ATUAL</div>
                    <div
                      className="state-value"
                      style={{
                        color: bloqueado ? "#e4483c" : "#35b37e",
                      }}
                    >
                      {estado.status || "SEM CONEXÃO"}
                    </div>
                    <div className="state-description">
                      {bloqueado
                        ? "Equipamento em condição de bloqueio."
                        : "Equipamento liberado para operação."}
                    </div>
                  </div>
                </div>
              </div>

              <div className="card connection">
                <div className="card-title-row">
                  <div className="card-title">
                    <Cable size={15} />
                    Conexão com o servidor
                  </div>
                  <div className="card-mini">API / SQLITE</div>
                </div>

                <div className="input-wrap">
                  <input
                    className="ip-input"
                    type="text"
                    value={ipInput}
                    onChange={(e) => {
                      setIpInput(e.target.value);
                      if (erroIp) setErroIp("");
                    }}
                    onKeyDown={(e) => {
                      if (e.key === "Enter") conectarESP();
                    }}
                    placeholder="192.168.1.50:3000"
                    aria-label="Endereço do servidor"
                    aria-invalid={erroIp ? "true" : "false"}
                  />

                  <button
                    className="connect"
                    onClick={conectarESP}
                    disabled={connecting}
                  >
                    {connecting ? "CONECTANDO..." : "CONECTAR"}
                  </button>
                </div>

                {espIp && offline && !connecting && (
                  <div className="error">
                    Não foi possível falar com o servidor nesse endereço.
                    Confira se o backend está rodando, o IP e o firewall
                    (porta 3000).
                  </div>
                )}

                {erroIp && <div className="error">{erroIp}</div>}

                {espIp && (
                  <div className="connection-details">
                    <div className="detail-row">
                      <span>Servidor</span>
                      <strong>{espIp}</strong>
                    </div>
                    <div className="detail-row">
                      <span>Endpoint</span>
                      <strong className="detail-url">
                        http://{espIp}/api/painel
                      </strong>
                    </div>
                    <button className="disconnect" onClick={desconectarESP}>
                      DESCONECTAR
                    </button>
                  </div>
                )}

                {mixedContent && (
                  <div className="warning">
                    <AlertTriangle size={14} />
                    <span>
                      Esta página usa HTTPS, mas o servidor responde em HTTP.
                      O navegador pode bloquear a conexão.
                    </span>
                  </div>
                )}

                {offline && espIp && (
                  <div className="offline">
                    <WifiOff size={14} />
                    Últimos dados conhecidos em exibição.
                  </div>
                )}

                {servidorOnline && estado.esp_online === false && (
                  <div className="offline">
                    <WifiOff size={14} />
                    <span>
                      O ESP32 não envia dados
                      {typeof estado.ultimo_contato_s === "number"
                        ? ` há ${estado.ultimo_contato_s} s`
                        : " ainda"}
                      . O status exibido é o último recebido.
                    </span>
                  </div>
                )}
              </div>

              <div className="metrics">
                <div className="metric">
                  <div className="metric-head">
                    <span>TRABALHADORES ATIVOS</span>
                    <Users className="metric-icon" size={16} />
                  </div>
                  <div className="metric-number">
                    {estado.trabalhadores_ativos ?? 0}
                  </div>
                  <div className="metric-note">Acessos atualmente ativos</div>
                </div>

                <div className="metric">
                  <div className="metric-head">
                    <span>REGISTROS</span>
                    <ListOrdered className="metric-icon" size={16} />
                  </div>
                  <div className="metric-number">
                    {estado.quantidade_logs ?? logs.length}
                  </div>
                  <div className="metric-note">Eventos registrados no sistema</div>
                </div>
              </div>

              <div className="card diag-card">
                <div className="card-title-row">
                  <div className="card-title">
                    <Radio size={15} />
                    Diagnóstico do sistema
                  </div>
                  <div className="card-mini">TEMPO REAL</div>
                </div>

                <div className="diag-status-list">
                  <div className="diag-status-item">
                    <div className="diag-status-left">
                      <MonitorSmartphone size={15} />
                      Touchscreen (GT911)
                    </div>
                    <span className={`diag-status-tag ${estado.touchscreen ? "" : "off"}`}>
                      <span className="status-dot" />
                      {estado.touchscreen ? "OK" : "INDISPONÍVEL"}
                    </span>
                  </div>

                  <div className="diag-status-item">
                    <div className="diag-status-left">
                      <Nfc size={15} />
                      Leitor RFID (PN532)
                    </div>
                    <span className={`diag-status-tag ${estado.rfid_disponivel ? "" : "off"}`}>
                      <span className="status-dot" />
                      {estado.rfid_disponivel ? "OK" : "NÃO DETECTADO"}
                    </span>
                  </div>

                  <div className="diag-status-item">
                    <div className="diag-status-left">
                      <Wifi size={15} />
                      Sinal Wi-Fi do ESP32
                    </div>
                    <span className="diag-status-tag">
                      {estado.wifi_rssi ? `${estado.wifi_rssi} dBm` : "—"}
                    </span>
                  </div>

                  <div className="diag-status-item">
                    <div className="diag-status-left">
                      <Activity size={15} />
                      Memória livre · reconexões Wi-Fi
                    </div>
                    <span className="diag-status-tag">
                      {estado.heap_livre
                        ? `${Math.round(estado.heap_livre / 1024)} KB`
                        : "—"}
                      {" · "}
                      {estado.wifi_reconexoes ?? 0}×
                    </span>
                  </div>
                </div>

                <div className="card-title-row" style={{ marginTop: 16, marginBottom: 0 }}>
                  <div className="card-title">
                    <Gauge size={15} />
                    Medição RFID · bancada
                  </div>
                </div>

                <div className="diag-measure-grid">
                  <div className="diag-measure-cell">
                    <div className="diag-measure-num">{estado.rfid_leituras_medidas ?? 0}</div>
                    <div className="diag-measure-lbl">LEITURAS</div>
                  </div>
                  <div className="diag-measure-cell">
                    <div className="diag-measure-num">
                      {estado.rfid_tempo_resposta_ultimo_ms ?? "—"}
                    </div>
                    <div className="diag-measure-lbl">ÚLTIMA (MS)</div>
                  </div>
                  <div className="diag-measure-cell">
                    <div className="diag-measure-num">
                      {typeof estado.rfid_tempo_resposta_medio_ms === "number"
                        ? estado.rfid_tempo_resposta_medio_ms.toFixed(1)
                        : "—"}
                    </div>
                    <div className="diag-measure-lbl">MÉDIA (MS)</div>
                  </div>
                  <div className="diag-measure-cell">
                    <div className="diag-measure-num">
                      {typeof estado.rfid_tempo_resposta_desvio_ms === "number"
                        ? estado.rfid_tempo_resposta_desvio_ms.toFixed(1)
                        : "—"}
                    </div>
                    <div className="diag-measure-lbl">DESVIO (MS)</div>
                  </div>
                </div>

                <div className="diag-measure-hint">
                  Média e desvio padrão calculados no próprio ESP32 a cada leitura.
                  Reinicie o equipamento para zerar a contagem antes de um novo teste.
                </div>
              </div>

              <div className="card logs-card" id="registros">
                <div className="logs-header">
                  <div className="logs-title">
                    <Activity size={16} />
                    Atividade recente
                  </div>
                  <div className="log-count">
                    {logs.length} ENTRADAS
                  </div>
                </div>

                <div className="logs-list">
                  {logs.length === 0 && (
                    <div className="empty">
                      Nenhum registro para este equipamento.
                    </div>
                  )}

                  {logs.map((log) => {
                    const isEntrada = log.evento === "ENTRADA";

                    return (
                      <div className="log-row" key={log.id}>
                        <div
                          className={`event-icon ${isEntrada ? "entrada" : "saida"
                            }`}
                        >
                          {isEntrada ? (
                            <LogIn size={16} />
                          ) : (
                            <LogOut size={16} />
                          )}
                        </div>

                        <div>
                          <div className="worker">
                            {log.trabalhador || "Trabalhador"}
                          </div>
                          <div className="function">
                            {log.funcao || "Função não informada"}
                          </div>
                          <div className="meta">
                            <span className="uid">
                              <Nfc size={10} />
                              {log.uid || "—"}
                            </span>
                            <span>
                              {log.data || "—"} {log.hora || ""}
                            </span>
                          </div>
                        </div>

                        <div
                          className={`event ${isEntrada ? "entrada" : "saida"
                            }`}
                        >
                          {log.evento || "EVENTO"}
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
            </section>
          ) : (
            <section className="equipment-page">
              <div className="card equipment-intro">
                <div className="equipment-label">INFRAESTRUTURA E-LOTO</div>
                <h2>Equipamentos</h2>
                <p>Componentes físicos utilizados no sistema E-LOTO, com imagem, função, informações técnicas e acesso ao produto.</p>
              </div>

              <div className="equipment-products">
                <article className="equipment-product-card">
                  <div className="product-image-wrap">
                    <img
                      src="https://cdn.awsli.com.br/600x450/1773/1773256/produto/301803853/69ab71c692633b28723159ae07ca9864-r7l2yvdgja.jpg"
                      alt="ESP32 com display TFT de 3,2 polegadas"
                      className="product-image"
                    />
                    <span className={`product-live ${estado.touchscreen ? "" : "off"}`}>
                      <span className="status-dot" /> {estado.touchscreen ? "TOUCH OK" : "TOUCH OFFLINE"}
                    </span>
                  </div>
                  <div className="product-body">
                    <div className="product-category">CONTROLADOR + INTERFACE</div>
                    <h3>ESP32 com Display 3,2&quot; Touch</h3>
                    <p className="product-description">Controlador principal do E-LOTO, responsável pela lógica do sistema, comunicação Wi-Fi e interface local pela tela touch.</p>
                    <div className="spec-grid">
                      <div><span>Microcontrolador</span><strong>ESP-WROOM-32</strong></div>
                      <div><span>Display</span><strong>TFT 3,2&quot;</strong></div>
                      <div><span>Resolução</span><strong>320 × 240</strong></div>
                      <div><span>Touch</span><strong>Capacitivo</strong></div>
                      <div><span>Comunicação</span><strong>SPI / Wi-Fi / Bluetooth</strong></div>
                      <div><span>Alimentação</span><strong>5V via USB-C</strong></div>
                    </div>
                    <div className="product-links">
                      <a href="https://www.mercadolivre.com.br/placa-desenvolvimento-esp32-com-display-32-pol-touch-nfe/p/MLB2087822007" target="_blank" rel="noreferrer" className="product-link primary">
                        Mercado Livre <ExternalLink size={13} />
                      </a>
                      <a href="https://www.robobuilders.com.br/placa-desenvolvimento-esp32-com-display-32-pol-touch-nfe" target="_blank" rel="noreferrer" className="product-link">
                        Ficha técnica <ExternalLink size={13} />
                      </a>
                    </div>
                  </div>
                </article>

                <article className="equipment-product-card">
                  <div className="product-image-wrap">
                    <img
                      src="https://www.elechouse.com/elechouse/images/product/PN532_module_V3/PN532-7.jpg"
                      alt="Módulo leitor RFID NFC PN532"
                      className="product-image"
                    />
                    <span className={`product-live ${estado.rfid_disponivel ? "" : "off"}`}>
                      <span className="status-dot" /> {estado.rfid_disponivel ? "LEITOR OK" : "NÃO DETECTADO"}
                    </span>
                  </div>
                  <div className="product-body">
                    <div className="product-category">IDENTIFICAÇÃO</div>
                    <h3>Leitor RFID / NFC PN532</h3>
                    <p className="product-description">Leitor utilizado para identificar os trabalhadores por cartão/tag NFC e enviar a identificação para o controlador ESP32.</p>
                    <div className="spec-grid">
                      <div><span>Controlador</span><strong>NXP PN532</strong></div>
                      <div><span>Frequência</span><strong>13,56 MHz</strong></div>
                      <div><span>Interfaces</span><strong>I²C / SPI / UART</strong></div>
                      <div><span>Alimentação</span><strong>3,3V – 5V</strong></div>
                      <div><span>Leituras medidas</span><strong>{estado.rfid_leituras_medidas ?? 0}</strong></div>
                      <div><span>Tempo médio</span><strong>
                        {typeof estado.rfid_tempo_resposta_medio_ms === "number"
                          ? `${estado.rfid_tempo_resposta_medio_ms.toFixed(1)} ms`
                          : "—"}
                      </strong></div>
                    </div>
                    <div className="product-links">
                      <a href="https://www.mercadolivre.com.br/kit-modulo-leitor-rfid-nfc-pn532-arduino/p/MLB2040592506?pdp_filters=item_id:MLB6163965734" target="_blank" rel="noreferrer" className="product-link primary">
                        Mercado Livre <ExternalLink size={13} />
                      </a>
                      <a href="https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/" target="_blank" rel="noreferrer" className="product-link">
                        Ficha técnica <ExternalLink size={13} />
                      </a>
                      <a href="https://www.elechouse.com/docs/pn532-v4/" target="_blank" rel="noreferrer" className="product-link">
                        Documentação <ExternalLink size={13} />
                      </a>
                    </div>
                  </div>
                </article>

                <article className="equipment-product-card">
                  <div className="product-image-wrap">
                    <img
                      src="https://images.prom.ua/5750719032_w700_h500_12v-elektromagnit-podemnyj.jpg"
                      alt="Eletroímã de retenção 12V"
                      className="product-image product-image-solenoid"
                    />
                    <span className={`product-live ${online ? "" : "off"}`}>
                      <span className="status-dot" /> {online ? (bloqueado ? "ACIONADO / BLOQUEIO" : "LIBERADO") : "SEM CONEXÃO"}
                    </span>
                  </div>
                  <div className="product-body">
                    <div className="product-category">ATUADOR DE BLOQUEIO</div>
                    <h3>Eletroímã 12V — P2015</h3>
                    <p className="product-description">Atuador eletromagnético utilizado no mecanismo físico de bloqueio da aplicação E-LOTO.</p>
                    <div className="spec-grid">
                      <div><span>Tensão</span><strong>12V DC</strong></div>
                      <div><span>Corrente</span><strong>250 mA</strong></div>
                      <div><span>Potência</span><strong>3 W</strong></div>
                      <div><span>Força informada</span><strong>25 kg</strong></div>
                      <div><span>Tipo</span><strong>Eletroímã</strong></div>
                      <div><span>Função</span><strong>Bloqueio físico</strong></div>
                    </div>
                    <div className="product-note">Imagem ilustrativa do formato de eletroímã P2015; confirme as especificações do seu exemplar antes da montagem.</div>
                    <div className="product-links">
                      <a href="https://www.mercadolivre.com.br/eletroima-12v-12-v-vdc-25-kg-p2015--3w-250-ma--0217/up/MLBU2821950150?pdp_filters=item_id:MLB834613756" target="_blank" rel="noreferrer" className="product-link primary">
                        Mercado Livre <ExternalLink size={13} />
                      </a>
                      <a href="https://www.smartkits.com.br/eletroima-p2015-12v-3kg" target="_blank" rel="noreferrer" className="product-link">
                        Referência técnica <ExternalLink size={13} />
                      </a>
                    </div>
                  </div>
                </article>

                <article className="equipment-product-card">
                  <div className="product-image-wrap">
                    <svg
                      className="product-image"
                      viewBox="0 0 300 200"
                      role="img"
                      aria-label="Módulo relé 1 canal 5V (ilustração)"
                    >
                      <rect x="24" y="28" width="252" height="144" rx="8" fill="#1f7a63" />
                      <circle cx="38" cy="42" r="5" fill="#f1f3f5" />
                      <circle cx="262" cy="42" r="5" fill="#f1f3f5" />
                      <circle cx="38" cy="158" r="5" fill="#f1f3f5" />
                      <circle cx="262" cy="158" r="5" fill="#f1f3f5" />
                      <rect x="30" y="76" width="22" height="48" rx="2" fill="#141414" />
                      <circle cx="41" cy="86" r="3.5" fill="#d4af37" />
                      <circle cx="41" cy="100" r="3.5" fill="#d4af37" />
                      <circle cx="41" cy="114" r="3.5" fill="#d4af37" />
                      <rect x="62" y="112" width="14" height="10" rx="1.5" fill="#222" />
                      <circle cx="72" cy="146" r="6" fill="#ff4d4d" />
                      <rect x="92" y="52" width="104" height="96" rx="6" fill="#2f5fb5" stroke="#1d3f80" strokeWidth="2" />
                      <rect x="106" y="66" width="76" height="8" rx="2" fill="#ffffff" opacity=".85" />
                      <rect x="106" y="82" width="56" height="6" rx="2" fill="#ffffff" opacity=".6" />
                      <text x="144" y="124" textAnchor="middle" fontSize="14" fontWeight="700" fill="#ffffff" fontFamily="monospace">5V · 10A</text>
                      <rect x="212" y="58" width="52" height="84" rx="4" fill="#2f78c9" stroke="#1d4f8f" strokeWidth="2" />
                      <circle cx="238" cy="76" r="9" fill="#d9dde2" stroke="#8a939c" strokeWidth="1.5" />
                      <circle cx="238" cy="100" r="9" fill="#d9dde2" stroke="#8a939c" strokeWidth="1.5" />
                      <circle cx="238" cy="124" r="9" fill="#d9dde2" stroke="#8a939c" strokeWidth="1.5" />
                      <line x1="232" y1="76" x2="244" y2="76" stroke="#6b747d" strokeWidth="2" />
                      <line x1="232" y1="100" x2="244" y2="100" stroke="#6b747d" strokeWidth="2" />
                      <line x1="232" y1="124" x2="244" y2="124" stroke="#6b747d" strokeWidth="2" />
                    </svg>
                    <span className={`product-live ${online ? "" : "off"}`}>
                      <span className="status-dot" /> {online ? (bloqueado ? "RELÉ ACIONADO" : "RELÉ DESLIGADO") : "SEM CONEXÃO"}
                    </span>
                  </div>
                  <div className="product-body">
                    <div className="product-category">ACIONAMENTO</div>
                    <h3>Módulo Relé 1 Canal 5V</h3>
                    <p className="product-description">Recebe o sinal do ESP32 e liga a alimentação de 12V do eletroímã quando o equipamento é bloqueado.</p>
                    <div className="spec-grid">
                      <div><span>Acionamento</span><strong>5V DC</strong></div>
                      <div><span>Contatos</span><strong>NA / NC / Comum</strong></div>
                      <div><span>Carga máxima</span><strong>10A · 250VAC / 30VDC</strong></div>
                      <div><span>Corrente da bobina</span><strong>15 ~ 20 mA</strong></div>
                      <div><span>Dimensões</span><strong>51 × 38 × 20 mm</strong></div>
                      <div><span>Indicação</span><strong>LED de status</strong></div>
                    </div>
                    <div className="product-note">Ilustração do módulo; confirme as especificações do seu exemplar antes da montagem.</div>
                    <div className="product-links">
                      <a href="https://www.mercadolivre.com.br/modulo-rele-arduino-1-canal-5v-iot-automacao-residencial/up/MLBU1475234390" target="_blank" rel="noreferrer" className="product-link primary">
                        Mercado Livre <ExternalLink size={13} />
                      </a>
                      <a href="https://www.eletrogate.com/modulo-rele-1-canal-5v" target="_blank" rel="noreferrer" className="product-link">
                        Referência técnica <ExternalLink size={13} />
                      </a>
                    </div>
                  </div>
                </article>
              </div>

              <div className="card equipment-detail">
                <div className="equipment-detail-title"><Server size={15} /> ESTADO DO SISTEMA</div>
                <div className="equipment-detail-row"><span>EQUIPAMENTO PRINCIPAL</span><strong>{estado.equipamento || "MESA PLANA"}</strong></div>
                <div className="equipment-detail-row"><span>SISTEMA</span><strong>{estado.sistema || "E-LOTO"}</strong></div>
                <div className="equipment-detail-row"><span>SERVIDOR</span><strong>{espIp || "Servidor não configurado"}</strong></div>
                <div className="equipment-detail-row"><span>STATUS</span><strong>{estado.status || "SEM CONEXÃO"}</strong></div>
                <div className="equipment-detail-row"><span>TRABALHADORES ATIVOS</span><strong>{estado.trabalhadores_ativos ?? 0}</strong></div>
                <div className="equipment-detail-row"><span>REGISTROS</span><strong>{estado.quantidade_logs ?? logs.length}</strong></div>
                <div className="equipment-detail-row"><span>TOUCHSCREEN</span><strong>{estado.touchscreen ? "OK" : "Indisponível"}</strong></div>
                <div className="equipment-detail-row"><span>LEITOR RFID</span><strong>{estado.rfid_disponivel ? "OK" : "Não detectado"}</strong></div>
              </div>
            </section>
          )}

          <div className="footer">
            <span>
              E-LOTO CONTROL CENTER • MONITORAMENTO LOCAL
            </span>
            <span>
              <Clock3 size={10} style={{ verticalAlign: "-1px" }} />{" "}
              {lastSync
                ? `ÚLTIMA SINCRONIZAÇÃO ${lastSync.toLocaleTimeString("pt-BR")}`
                : "AGUARDANDO CONEXÃO"}
            </span>
          </div>
        </main>
      </div>
    </div>
  );
}
