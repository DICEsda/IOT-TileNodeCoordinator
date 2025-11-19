import React, { useMemo } from 'react';
import { Line } from '@react-three/drei';
import * as THREE from 'three';

const mmToMeters = (value = 0) => value / 1000;

export default function TargetTrail({ history, targetId, color }) {
  const points = useMemo(() => {
    if (!Array.isArray(history) || history.length === 0) {
      return [];
    }
    const trail = [];
    history.forEach((frame) => {
      const target = frame?.targets?.find((entry) => entry?.id === targetId && entry?.valid);
      if (target) {
        const pos = target.position_mm ?? target.position ?? {};
        const x = mmToMeters(pos.x ?? target.x ?? 0);
        const z = mmToMeters(pos.y ?? target.y ?? 0);
        trail.push(new THREE.Vector3(x, 0.01, z));
      }
    });
    return trail;
  }, [history, targetId]);

  if (points.length < 2) {
    return null;
  }

  return <Line points={points} color={color} lineWidth={1.5} />;
}
