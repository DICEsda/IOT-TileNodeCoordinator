import { useEffect, useRef, useState } from 'react';

const stripTrailingSlash = (value) => value.replace(/\/+$/, '');

const resolveHttpUrl = (host, path) => {
  if (!host) {
    return null;
  }
  const trimmed = stripTrailingSlash(host.trim());
  if (/^https?:\/\//i.test(trimmed)) {
    return `${trimmed}${path}`;
  }
  if (/^wss?:\/\//i.test(trimmed)) {
    const url = new URL(trimmed.replace(/^ws/i, 'http'));
    return `${url.protocol}//${url.host}${path}`;
  }
  return `http://${trimmed}${path}`;
};

const resolveWsUrl = (host, path) => {
  if (!host) {
    return null;
  }
  const trimmed = stripTrailingSlash(host.trim());
  if (/^https?:\/\//i.test(trimmed)) {
    const url = new URL(trimmed);
    const scheme = url.protocol === 'https:' ? 'wss:' : 'ws:';
    return `${scheme}//${url.host}${path}`;
  }
  if (/^wss?:\/\//i.test(trimmed)) {
    return `${trimmed}${path}`;
  }
  const isSecure = window.location.protocol === 'https:';
  const scheme = isSecure ? 'wss://' : 'ws://';
  return `${scheme}${trimmed}${path}`;
};

export function useRadarSocket(initialHost = '', options = {}) {
  const { historyLimit = 360 } = options;
  const [host, setHostState] = useState(initialHost.trim());
  const [connected, setConnected] = useState(false);
  const [latestFrame, setLatestFrame] = useState(null);
  const [history, setHistory] = useState([]);
  const [error, setError] = useState(null);
  const [reconnectAttempt, setReconnectAttempt] = useState(0);
  const reconnectTimerRef = useRef();
  const socketRef = useRef();

  useEffect(() => {
    if (!host) {
      setConnected(false);
      setLatestFrame(null);
      setHistory([]);
      return;
    }

    let intentionalClose = false;
    const url = resolveWsUrl(host, '/radar');
    const ws = new WebSocket(url);
    socketRef.current = ws;
    setError(null);

    ws.onopen = () => {
      setConnected(true);
    };

    ws.onmessage = (event) => {
      try {
        const payload = JSON.parse(event.data);
        setLatestFrame(payload);
        setHistory((prev) => {
          const preserved = prev.length
            ? prev.slice(Math.max(prev.length - (historyLimit - 1), 0))
            : [];
          const next = preserved;
          if (next.length && next[next.length - 1]?.timestamp_ms === payload.timestamp_ms) {
            next[next.length - 1] = payload;
          } else {
            next.push(payload);
          }
          return next;
        });
      } catch (parseError) {
        console.error('Failed to parse radar payload', parseError);
        setError('Received malformed frame');
      }
    };

    ws.onerror = (evt) => {
      console.warn('WebSocket error', evt);
      setError('WebSocket error');
    };

    ws.onclose = () => {
      setConnected(false);
      if (!intentionalClose) {
        reconnectTimerRef.current = setTimeout(() => {
          setError('Connection lost, retrying...');
          setReconnectAttempt((value) => value + 1);
        }, 1000);
      }
    };

    return () => {
      intentionalClose = true;
      if (reconnectTimerRef.current) {
        clearTimeout(reconnectTimerRef.current);
        reconnectTimerRef.current = undefined;
      }
      if (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING) {
        ws.close(1000, 'cleanup');
      }
    };
  }, [host, historyLimit, reconnectAttempt]);

  useEffect(() => {
    if (!host) {
      return undefined;
    }
    const abortController = new AbortController();
    const url = resolveHttpUrl(host, '/api/history');
    fetch(url, { signal: abortController.signal })
      .then(async (response) => {
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }
        return response.json();
      })
      .then((payload) => {
        if (Array.isArray(payload)) {
          setHistory(payload.slice(-historyLimit));
          if (payload.length) {
            setLatestFrame(payload[payload.length - 1]);
          }
        }
      })
      .catch((fetchError) => {
        if (fetchError.name !== 'AbortError') {
          console.warn('Failed to fetch history', fetchError);
          setError('Failed to fetch history');
        }
      });

    return () => abortController.abort();
  }, [host, historyLimit]);

  return {
    host,
    setHost: (value) => {
      const trimmed = value.trim().replace(/\s+/g, '');
      setReconnectAttempt(0);
      setHostState(trimmed);
    },
    connected,
    latestFrame,
    history,
    error
  };
}

export default useRadarSocket;
