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

function updateUI(status) {
  if (!status) return;

  // Power badge
  const pw = document.getElementById('status-power');
  pw.textContent = status.power === 'on' ? 'EIN' : status.power === 'off' ? 'AUS' : '–';
  pw.style.background = status.power === 'on' ? '#1e8e3e' : 'rgba(255,255,255,0.2)';

  // Input badge
  const inp = document.getElementById('status-input');
  inp.textContent = status.input !== 'unknown' ? status.input.toUpperCase() : '–';

  // Blank badge
  const bl = document.getElementById('status-blank');
  bl.classList.toggle('hidden', !status.blank);

  // Offline badge
  const rc = document.getElementById('status-reach');
  rc.classList.toggle('hidden', status.reachable !== false);

  // Highlight active input button
  document.querySelectorAll('[data-input]').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.input === status.input);
  });
}

async function fetchStatus() {
  const data = await apiFetch('/api/status');
  updateUI(data);
}

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

// Add data-input attributes to input buttons for active-state highlighting
document.querySelectorAll('.btn').forEach(btn => {
  const onclick = btn.getAttribute('onclick') || '';
  const m = onclick.match(/setInput\('(\w+)'\)/);
  if (m) btn.dataset.input = m[1];
});

// Initial fetch + polling
fetchStatus();
setInterval(fetchStatus, POLL_MS);
