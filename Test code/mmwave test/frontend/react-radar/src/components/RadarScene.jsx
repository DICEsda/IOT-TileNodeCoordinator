import React, { useMemo } from 'react';
import { Canvas } from '@react-three/fiber';
import { Html, OrbitControls } from '@react-three/drei';
import * as THREE from 'three';
import TargetTrail from './TargetTrail.jsx';

const TARGET_COLORS = ['#00d8ff', '#ff9100', '#d500f9'];
const mmToMeters = (value = 0) => value / 1000;

const buildPosition = (target) => {
  const pos = target?.position_mm ?? target?.position ?? {};
  const x = mmToMeters(pos.x ?? target?.x ?? 0);
  const z = mmToMeters(pos.y ?? target?.y ?? 0);
  return [x, 0, z];
};

function VelocityVector({ target, color }) {
  const velocity = target?.velocity_m_s ?? target?.velocity;
  const pos = target?.position_mm ?? target?.position;
  if (!target?.valid || !velocity || !pos) {
    return null;
  }

  const lineData = useMemo(() => {
    const baseVec = new THREE.Vector3(...buildPosition(target));
    const velocityVec = new THREE.Vector3(velocity.x ?? 0, velocity.z ?? 0, velocity.y ?? 0);
    const tipVec = baseVec.clone().add(velocityVec.multiplyScalar(0.5));
    if (baseVec.distanceTo(tipVec) < 0.01) {
      return null;
    }
    return {
      baseVec,
      tipVec,
      buffer: new Float32Array([...baseVec.toArray(), ...tipVec.toArray()]),
      tipArray: [tipVec.x, tipVec.y, tipVec.z]
    };
  }, [target, velocity]);

  if (!lineData) {
    return null;
  }

  return (
    <group>
      <line>
        <bufferGeometry>
          <bufferAttribute
            attach="attributes-position"
            count={2}
            array={lineData.buffer}
            itemSize={3}
          />
        </bufferGeometry>
        <lineBasicMaterial color={color} linewidth={2} />
      </line>
      <mesh position={lineData.tipArray}>
        <coneGeometry args={[0.04, 0.12, 14]} />
        <meshStandardMaterial color={color} />
      </mesh>
    </group>
  );
}

export default function RadarScene({ frame, history }) {
  const targets = frame?.targets ?? [];

  return (
    <div className="radar-scene">
      <Canvas camera={{ position: [0, 2.5, 5.5], fov: 55 }}>
        <color attach="background" args={["#060708"]} />
        <fog attach="fog" args={["#060708", 6, 14]} />
        <ambientLight intensity={0.4} />
        <directionalLight position={[3, 6, 5]} intensity={0.7} />
        <pointLight position={[-4, 5, -3]} intensity={0.4} />
        <gridHelper args={[10, 20, '#1b2735', '#22303c']} />
        <axesHelper args={[1]} />

        <mesh position={[0, -0.02, 0]}>
          <cylinderGeometry args={[0.04, 0.04, 0.02, 16]} />
          <meshStandardMaterial color="#ffffff" emissive="#7f8fa6" emissiveIntensity={0.4} />
        </mesh>

        {targets.map((target, index) => {
          const color = TARGET_COLORS[index % TARGET_COLORS.length];
          const position = buildPosition(target);
          return (
            <group key={target?.id ?? index}>
              {target?.valid ? (
                <mesh position={[position[0], 0.05, position[2]]}>
                  <sphereGeometry args={[0.08, 32, 32]} />
                  <meshStandardMaterial color={color} emissive={color} emissiveIntensity={0.6} />
                </mesh>
              ) : (
                <mesh position={[position[0], 0.03, position[2]]}>
                  <sphereGeometry args={[0.06, 24, 24]} />
                  <meshStandardMaterial color="#2d3436" emissive="#2c3e50" emissiveIntensity={0.1} />
                </mesh>
              )}
              <TargetTrail history={history} targetId={target?.id} color={color} />
              <VelocityVector target={target} color={color} />
              <Html
                position={[position[0], 0.18, position[2]]}
                center
                className={`target-label ${target?.valid ? 'target-label--active' : ''}`}
              >
                {`ID ${target?.id ?? index + 1}`}
              </Html>
            </group>
          );
        })}

        <OrbitControls enableDamping dampingFactor={0.08} maxPolarAngle={Math.PI / 2.1} />
      </Canvas>
      {!frame ? <div className="radar-placeholder">Waiting for frames from the ESP32-S3…</div> : null}
    </div>
  );
}
