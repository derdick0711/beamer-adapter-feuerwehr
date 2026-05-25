'use strict';

const POLL_MS = 5000;

let toastTimer = null;

function showToast(msg, isError = false) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.style.background = isError ? '#b31412' : '#323232';
  t.classList.remove('hidden');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.add('hidden'), 3000);
}

async function apiFetch(path, method = 'GET', body = null) {
  try {
    const opts = { method, headers: {} };
    if (body) {
      opts.body = JSON.stringify(body);
      opts.headers['Content-Type'] = 'application/json';
    }
    const res = await fetch(path, opts);
    const data = await res.json();
    if (!res.ok) {
      showToast(data.error === 'rs232_timeout'
        ? 'Beamer antwortet nicht (RS232 Timeout)'
        : `Fehler: ${data.error || res.status}`, true);
      return null;
    }
    return data;
  } catch (e) {
    showToast('Verbindung zum Adapter fehlgeschlagen', true);
    return null;
  }
}

// ── Beamer status ─────────────────────────────────────────────────────────────

function updateUI(status) {
  if (!status) return;

  const pw = document.getElementById('status-power');
  pw.textContent = status.power === 'on' ? 'EIN' : status.power === 'off' ? 'AUS' : '–';
  pw.style.background = status.power === 'on' ? '#1e8e3e' : 'rgba(255,255,255,0.2)';

  const inp = document.getElementById('status-input');
  inp.textContent = status.input !== 'unknown' ? status.input.toUpperCase() : '–';

  document.getElementById('status-blank').classList.toggle('hidden', !status.blank);
  document.getElementById('status-reach').classList.toggle('hidden', status.reachable !== false);

  const ipEl = document.getElementById('status-ip');
  if (ipEl && status.ip && status.ip !== '0.0.0.0') {
    ipEl.textContent = status.ip;
    ipEl.classList.remove('hidden');
  }

  const mqttEl = document.getElementById('status-mqtt');
  if (mqttEl) {
    mqttEl.textContent = status.mqttConnected ? 'MQTT: ON' : 'MQTT: OFF';
    mqttEl.classList.toggle('hidden', !status.mqttConnected);
  }

  document.querySelectorAll('[data-input]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.input === status.input);
  });
}

async function fetchStatus() {
  const data = await apiFetch('/api/status');
  updateUI(data);
}

// ── Beamer commands ───────────────────────────────────────────────────────────

async function power(state) {
  const data = await apiFetch('/api/power', 'POST', { state });
  updateUI(data);
  if (data) showToast(state === 'on' ? 'Beamer eingeschaltet' : 'Beamer ausgeschaltet');
}

async function setInput(input) {
  const data = await apiFetch('/api/input', 'POST', { input });
  updateUI(data);
  if (data) showToast(`Eingang: ${input.toUpperCase()}`);
}

async function blank(enabled) {
  const data = await apiFetch('/api/blank', 'POST', { enabled });
  updateUI(data);
  if (data) showToast(enabled ? 'Bild schwarz geschaltet' : 'Bild freigegeben');
}

// ── Szenen ────────────────────────────────────────────────────────────────────

async function startScene() {
  showToast('Szene wird gestartet…');
  const data = await apiFetch('/api/scene/start', 'POST');
  if (!data) return;
  const ok = data.success;
  showToast(ok ? `Präsentation gestartet (${data.duration_ms} ms)`
               : 'Präsentation gestartet (Teilfehler)', !ok);
  fetchLightStatus();
  fetchScreenStatus();
  fetchStatus();
}

async function stopScene() {
  showToast('Szene wird beendet…');
  const data = await apiFetch('/api/scene/stop', 'POST');
  if (!data) return;
  const ok = data.success;
  showToast(ok ? `Präsentation beendet (${data.duration_ms} ms)`
               : 'Präsentation beendet (Teilfehler)', !ok);
  fetchLightStatus();
  fetchScreenStatus();
  fetchStatus();
}

// ── Deckenlicht ───────────────────────────────────────────────────────────────

function updateLightUI(status) {
  if (!status) return;
  const dot = document.getElementById('light-badge');
  if (dot) {
    dot.className = 'status-dot ' + (status.output ? 'on' : 'off');
    dot.title = status.reachable ? (status.output ? 'an' : 'aus') : 'nicht erreichbar';
  }
}

async function fetchLightStatus() {
  const data = await apiFetch('/api/light/status');
  updateLightUI(data);
}

async function setLight(state) {
  const data = await apiFetch('/api/light', 'POST', { state });
  if (data) {
    showToast(state === 'on' ? 'Licht eingeschaltet' : 'Licht ausgeschaltet');
    fetchLightStatus();
  }
}

// ── Leinwand ──────────────────────────────────────────────────────────────────

function updateScreenUI(status) {
  if (!status) return;
  const lbl = document.getElementById('screen-pos');
  if (lbl) {
    if (!status.reachable) {
      lbl.textContent = '(nicht erreichbar)';
    } else {
      lbl.textContent = `${status.current_pos}% – ${status.state}`;
    }
  }
}

async function fetchScreenStatus() {
  const data = await apiFetch('/api/screen/status');
  updateScreenUI(data);
}

async function setScreen(action, pos) {
  const body = pos !== undefined ? { action, pos } : { action };
  const data = await apiFetch('/api/screen', 'POST', body);
  if (data) {
    const labels = { open: 'Leinwand hoch', close: 'Leinwand runter', stop: 'Leinwand gestoppt' };
    showToast(labels[action] || `Leinwand: ${action}`);
    setTimeout(fetchScreenStatus, 500);
  }
}

// ── Init: data-input attributes + polling ────────────────────────────────────

document.querySelectorAll('.btn').forEach(btn => {
  const onclick = btn.getAttribute('onclick') || '';
  const m = onclick.match(/setInput\('(\w+)'\)/);
  if (m) btn.dataset.input = m[1];
});

fetchStatus();
fetchLightStatus();
fetchScreenStatus();
setInterval(fetchStatus, POLL_MS);
setInterval(fetchLightStatus, POLL_MS);
setInterval(fetchScreenStatus, POLL_MS);
