import React, { useMemo, useState } from 'react';
import RadarScene from './components/RadarScene.jsx';
import { useRadarSocket } from './hooks/useRadarSocket.js';

const DEFAULT_HISTORY_LIMIT = 300;

const formatDistance = (distanceMm) => {
  if (distanceMm == null) {
    return '—';
  }
  if (distanceMm >= 1000) {
    return `${(distanceMm / 1000).toFixed(2)} m`;
  }
  return `${distanceMm} mm`;
};

const formatSpeed = (speedCmS) => {
  if (speedCmS == null) {
    return '—';
  }
  return `${(speedCmS / 100.0).toFixed(2)} m/s`;
};

const formatVector = (vector) => {
  if (!vector) {
    return '—';
  }
  const { x = 0, y = 0, z = 0 } = vector;
  return `(${x.toFixed(2)}, ${y.toFixed(2)}, ${z.toFixed(2)}) m/s`;
};

export default function App() {
  const defaultHost = (typeof window !== 'undefined' && window.location.hostname) || '';
  const [hostInput, setHostInput] = useState(defaultHost);
  const [historyWindow, setHistoryWindow] = useState(150);
  const { host, setHost, connected, latestFrame, history, error } = useRadarSocket(defaultHost, {
    historyLimit: DEFAULT_HISTORY_LIMIT
  });

  const visibleHistory = useMemo(() => {
    if (!historyWindow || historyWindow >= history.length) {
      return history;
    }
    return history.slice(-historyWindow);
  }, [history, historyWindow]);

  const targets = latestFrame?.targets ?? [];

  const handleHostSubmit = (event) => {
    event.preventDefault();
    setHost(hostInput);
  };

  return (
    <div className="app">
      <header className="app__header">
        <div>
          <h1>HLK-LD2450 3D Radar Visualizer</h1>
          <p className="app__subtitle">Capture multiple targets, history, and velocity vectors in real time.</p>
        </div>
        <form className="connection-form" onSubmit={handleHostSubmit}>
          <label htmlFor="host-input">ESP32-S3 host/IP</label>
          <div className="connection-form__row">
            <input
              id="host-input"
              type="text"
              value={hostInput}
              onChange={(event) => setHostInput(event.target.value)}
              placeholder="192.168.4.1"
              autoComplete="off"
            />
            <button type="submit">Connect</button>
          </div>
          <small>Provide an IP (optionally with protocol). The app opens a WebSocket on /radar.</small>
        </form>
        <div className="status-line">
          <span className={`status-dot ${connected ? 'status-dot--online' : 'status-dot--offline'}`} aria-hidden />
          <span>{connected ? `Connected to ${host || 'device'}` : 'Disconnected'}</span>
          {error ? <span className="status-line__error">{error}</span> : null}
        </div>
      </header>

      <main className="layout">
        <section className="scene-panel">
          {connected ? (
            <RadarScene frame={latestFrame} history={visibleHistory} />
          ) : (
            <div className="connection-gate">
              <h2>Device Not Connected</h2>
              <p>Enter the ESP32 IP or hostname above and press Connect to begin streaming radar data.</p>
              <p className="hint">Default AP mode IP: <code>192.168.4.1</code></p>
            </div>
          )}
        </section>
        <aside className="detail-panel">
          <div className="detail-panel__header">
            <h2>Targets</h2>
            <div className="history-control">
              <label htmlFor="history-range">History window ({historyWindow})</label>
              <input
                id="history-range"
                type="range"
                min="20"
                max={DEFAULT_HISTORY_LIMIT}
                value={historyWindow}
                onChange={(event) => setHistoryWindow(Number(event.target.value))}
              />
            </div>
          </div>
          <table className="targets-table">
            <thead>
              <tr>
                <th>ID</th>
                <th>Status</th>
                <th>Distance</th>
                <th>Speed</th>
                <th>Velocity (x, y, z)</th>
              </tr>
            </thead>
            <tbody>
              {connected && targets.length ? (
                targets.map((target) => (
                  <tr key={target.id} className={target.valid ? 'targets-table__row--valid' : 'targets-table__row--invalid'}>
                    <td>{target.id}</td>
                    <td>{target.valid ? 'Valid' : 'Idle'}</td>
                    <td>{formatDistance(target.distance_mm)}</td>
                    <td>{formatSpeed(target.speed_cm_s)}</td>
                    <td>{formatVector(target.velocity_m_s)}</td>
                  </tr>
                ))
              ) : (
                <tr>
                  <td colSpan={5} className="targets-table__empty">
                    {connected ? 'Waiting for radar frames…' : 'Connect to device to view targets'}
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </aside>
      </main>
    </div>
  );
}
