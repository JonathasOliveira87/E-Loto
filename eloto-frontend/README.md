# E-LOTO — dashboard (front)

React + Vite. Lê tudo do backend (`GET /api/painel`); não conversa com o ESP32.

```bash
npm install
npm run dev      # desenvolvimento
npm run build    # gera dist/
```

- **Servido pelo backend** (copie `dist/` para `eloto-backend/public/`): abra
  `http://IP-DO-PC:3000`. O endereço do servidor é detectado sozinho.
- **Em desenvolvimento** (`npm run dev`, porta 5173): no campo "Conexão com o
  servidor" digite o endereço do backend, ex.: `192.168.1.50:3000`.
