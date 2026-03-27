// ─── State ───────────────────────────────────────────────────────────────────
let port       = null;
let reader     = null;
let writer     = null;
let readLoop   = null;
let connected  = false;
let rxBytes    = 0;

// ─── DOM Refs ─────────────────────────────────────────────────────────────────
const output       = document.getElementById('output');
const inputField   = document.getElementById('inputField');
const sendBtn      = document.getElementById('sendBtn');
const connectBtn   = document.getElementById('connectBtn');
const clearBtn     = document.getElementById('clearBtn');
const portBadge    = document.getElementById('portBadge');
const statusMsg    = document.getElementById('statusMsg');
const byteCount    = document.getElementById('byteCount');
const themeBtn     = document.getElementById('themeBtn');
const themeIcon    = document.getElementById('themeIcon');
const fullscreenBtn= document.getElementById('fullscreenBtn');
const fsIcon       = document.getElementById('fsIcon');
const app          = document.getElementById('app');

// ─── Theme ────────────────────────────────────────────────────────────────────
let isDark = true;
themeBtn.addEventListener('click', () => {
  isDark = !isDark;
  document.documentElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
  themeIcon.textContent = isDark ? '☀' : '☾';
});

// ─── Fullscreen ───────────────────────────────────────────────────────────────
fullscreenBtn.addEventListener('click', () => {
  if (!document.fullscreenElement) {
    app.requestFullscreen().catch(() => {});
  } else {
    document.exitFullscreen();
  }
});

document.addEventListener('fullscreenchange', () => {
  if (document.fullscreenElement) {
    fsIcon.textContent = '⛶';
    fsIcon.style.transform = 'rotate(45deg)';
  } else {
    fsIcon.textContent = '⛶';
    fsIcon.style.transform = '';
  }
});

// ─── Clear ────────────────────────────────────────────────────────────────────
clearBtn.addEventListener('click', () => {
  output.innerHTML = '<span class="sys">[MiniConsole] Ausgabe geleert.\n</span>';
  rxBytes = 0;
  updateByteCount();
});

// ─── Connect / Disconnect ─────────────────────────────────────────────────────
connectBtn.addEventListener('click', async () => {
  if (connected) {
    await disconnect();
  } else {
    await connect();
  }
});

async function connect() {
  if (!('serial' in navigator)) {
    appendSys('[Fehler] Web Serial API nicht verfügbar. Bitte Chrome oder Edge verwenden.');
    return;
  }

  try {
    port = await navigator.serial.requestPort();

    const baud    = parseInt(document.getElementById('baudRate').value);
    const data    = parseInt(document.getElementById('dataBits').value);
    const stop    = parseInt(document.getElementById('stopBits').value);
    const parity  = document.getElementById('parity').value;

    await port.open({ baudRate: baud, dataBits: data, stopBits: stop, parity });

    const info = port.getInfo();
    const label = info.usbVendorId
      ? `VID:${info.usbVendorId.toString(16).toUpperCase()} PID:${info.usbProductId.toString(16).toUpperCase()}`
      : 'Serial Port';

    connected = true;
    setConnectedState(true, label, baud);
    appendSys(`[MiniConsole] Verbunden mit ${label} @ ${baud} Baud`);

    writer = port.writable.getWriter();
    startReadLoop();

  } catch (err) {
    if (err.name !== 'NotFoundError') {
      appendSys(`[Fehler] ${err.message}`);
    }
  }
}

async function disconnect() {
  try {
    connected = false;
    setConnectedState(false);

    if (reader) {
      await reader.cancel();
      reader = null;
    }
    if (writer) {
      await writer.close();
      writer = null;
    }
    if (port) {
      await port.close();
      port = null;
    }

    appendSys('[MiniConsole] Verbindung getrennt.');
  } catch (err) {
    appendSys(`[Fehler beim Trennen] ${err.message}`);
  }
}

// ─── Read Loop ────────────────────────────────────────────────────────────────
function startReadLoop() {
  const decoder = new TextDecoderStream();
  port.readable.pipeTo(decoder.writable);
  reader = decoder.readable.getReader();

  (async () => {
    try {
      while (connected) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          rxBytes += new TextEncoder().encode(value).length;
          updateByteCount();
          appendData(value);
        }
      }
    } catch (err) {
      if (connected) {
        appendSys(`[Verbindung verloren] ${err.message}`);
        await disconnect();
      }
    }
  })();
}

// ─── Send ─────────────────────────────────────────────────────────────────────
async function sendData() {
  if (!writer || !connected) return;
  const text = inputField.value;
  if (!text) return;

  const ending = document.getElementById('lineEnding').value
    .replace('\\n', '\n').replace('\\r', '\r');

  const encoded = new TextEncoder().encode(text + ending);
  try {
    await writer.write(encoded);
    appendEcho(text);
    inputField.value = '';
  } catch (err) {
    appendSys(`[Sendefehler] ${err.message}`);
  }
}

sendBtn.addEventListener('click', sendData);
inputField.addEventListener('keydown', (e) => {
  if (e.key === 'Enter') sendData();
  if (e.key === 'ArrowUp') recallHistory(-1);
  if (e.key === 'ArrowDown') recallHistory(1);
});

// ─── Command History ──────────────────────────────────────────────────────────
const history = [];
let histIdx = -1;

function recallHistory(dir) {
  if (history.length === 0) return;
  histIdx = Math.max(0, Math.min(history.length - 1, histIdx + dir));
  inputField.value = history[histIdx] || '';
}

// ─── Output Helpers ───────────────────────────────────────────────────────────
const MAX_LINES = 2000;

function appendData(text) {
  const span = document.createElement('span');
  span.className = 'rx';
  span.textContent = text;
  output.appendChild(span);
  trimOutput();
  scrollBottom();
}

function appendEcho(text) {
  history.unshift(text);
  histIdx = -1;
  const span = document.createElement('span');
  span.className = 'tx';
  span.textContent = '➤ ' + text + '\n';
  output.appendChild(span);
  trimOutput();
  scrollBottom();
}

function appendSys(text) {
  const span = document.createElement('span');
  span.className = 'sys';
  span.textContent = text + '\n';
  output.appendChild(span);
  trimOutput();
  scrollBottom();
}

function trimOutput() {
  const lines = output.querySelectorAll('span');
  if (lines.length > MAX_LINES) {
    for (let i = 0; i < lines.length - MAX_LINES; i++) {
      lines[i].remove();
    }
  }
}

function scrollBottom() {
  output.parentElement.scrollTop = output.parentElement.scrollHeight;
}

function updateByteCount() {
  byteCount.textContent = `RX: ${formatBytes(rxBytes)}`;
}

function formatBytes(b) {
  if (b < 1024) return b + ' B';
  if (b < 1048576) return (b / 1024).toFixed(1) + ' KB';
  return (b / 1048576).toFixed(2) + ' MB';
}

// ─── UI State ─────────────────────────────────────────────────────────────────
function setConnectedState(on, label = '', baud = '') {
  if (on) {
    connectBtn.textContent  = 'DISCONNECT';
    connectBtn.classList.add('active');
    portBadge.textContent   = `● ${label}`;
    portBadge.classList.add('connected');
    statusMsg.textContent   = `Verbunden @ ${baud} Baud`;
    inputField.disabled     = false;
    sendBtn.disabled        = false;
    inputField.focus();
  } else {
    connectBtn.textContent  = 'CONNECT';
    connectBtn.classList.remove('active');
    portBadge.textContent   = 'DISCONNECTED';
    portBadge.classList.remove('connected');
    statusMsg.textContent   = 'Nicht verbunden';
    inputField.disabled     = true;
    sendBtn.disabled        = true;
  }
}
