import { useEffect, useRef, useState } from "react";
import Grenade from "./grenade";
import {
  buildGrenadeKey,
  getFlashPulseDurationMs,
  getLandedPersistMs,
  getThrownPersistMs,
  GRENADE_RENDER_STATE,
  normalizeGrenadeType,
} from "../utilities/grenadeVisuals";

const normalizeGrenade = (grenade) => ({
  ...grenade,
  m_type: normalizeGrenadeType(grenade?.m_type),
});

const GrenadeLayer = ({ grenadeData, mapData, settings, averageLatency, radarImage }) => {
  const cacheRef = useRef({
    thrown: new Map(),
    landed: new Map(),
    previousThrown: new Map(),
    pulses: [],
  });

  const [renderData, setRenderData] = useState({
    thrown: [],
    landed: [],
    pulses: [],
  });

  useEffect(() => {
    const now = Date.now();
    const incomingThrown = (grenadeData?.thrown || []).map(normalizeGrenade);
    const incomingLanded = (grenadeData?.landed || []).map(normalizeGrenade);

    const incomingThrownKeys = new Set();
    const incomingLandedKeys = new Set();

    incomingThrown.forEach((grenade) => {
      const cacheKey = buildGrenadeKey(grenade, "thrown");
      incomingThrownKeys.add(cacheKey);

      cacheRef.current.thrown.set(cacheKey, {
        ...cacheRef.current.thrown.get(cacheKey),
        ...grenade,
        cacheKey,
        renderState: GRENADE_RENDER_STATE.THROWN,
        lastSeenAt: now,
        expiresAt: now + getThrownPersistMs(grenade.m_type, settings),
      });
    });

    incomingLanded.forEach((grenade) => {
      const cacheKey = buildGrenadeKey(grenade, "landed");
      incomingLandedKeys.add(cacheKey);
      cacheRef.current.landed.set(cacheKey, {
        ...cacheRef.current.landed.get(cacheKey),
        ...grenade,
        cacheKey,
        renderState:
          grenade.m_type === "molo"
            ? GRENADE_RENDER_STATE.BURNING
            : GRENADE_RENDER_STATE.SMOKE_ACTIVE,
        lastSeenAt: now,
        expiresAt: now + getLandedPersistMs(grenade.m_type, settings),
      });
    });

    if (settings.flashPulseEnabled !== false) {
      cacheRef.current.previousThrown.forEach((grenade, cacheKey) => {
        if (grenade.m_type !== "flashbang" || incomingThrownKeys.has(cacheKey)) {
          return;
        }

        cacheRef.current.pulses.push({
          ...grenade,
          renderState: GRENADE_RENDER_STATE.FLASH_PULSE,
          cacheKey: `flash-pulse:${cacheKey}:${now}`,
          expiresAt: now + getFlashPulseDurationMs(settings),
        });
      });
    }

    cacheRef.current.previousThrown = new Map(
      incomingThrown.map((grenade) => {
        const cacheKey = buildGrenadeKey(grenade, "thrown");
        return [cacheKey, { ...grenade, cacheKey }];
      })
    );

    cacheRef.current.thrown.forEach((grenade, cacheKey) => {
      if (!incomingThrownKeys.has(cacheKey) && grenade.expiresAt <= now) {
        cacheRef.current.thrown.delete(cacheKey);
      }
    });

    cacheRef.current.landed.forEach((grenade, cacheKey) => {
      if (!incomingLandedKeys.has(cacheKey) && grenade.expiresAt <= now) {
        cacheRef.current.landed.delete(cacheKey);
      }
    });

    cacheRef.current.pulses = cacheRef.current.pulses.filter((pulse) => pulse.expiresAt > now);

    setRenderData({
      thrown: [...cacheRef.current.thrown.values()],
      landed: [...cacheRef.current.landed.values()],
      pulses: [...cacheRef.current.pulses],
    });
  }, [grenadeData, settings]);

  return (
    <>
      {renderData.landed.map((grenade) => (
        <Grenade
          key={grenade.cacheKey}
          grenadeData={grenade}
          mapData={mapData}
          settings={settings}
          averageLatency={averageLatency}
          radarImage={radarImage}
          renderState={grenade.renderState}
        />
      ))}

      {renderData.thrown.map((grenade) => (
        <Grenade
          key={grenade.cacheKey}
          grenadeData={grenade}
          mapData={mapData}
          settings={settings}
          averageLatency={averageLatency}
          radarImage={radarImage}
          renderState={GRENADE_RENDER_STATE.THROWN}
        />
      ))}

      {renderData.pulses.map((grenade) => (
        <Grenade
          key={grenade.cacheKey}
          grenadeData={grenade}
          mapData={mapData}
          settings={settings}
          averageLatency={averageLatency}
          radarImage={radarImage}
          renderState={GRENADE_RENDER_STATE.FLASH_PULSE}
        />
      ))}
    </>
  );
};

export default GrenadeLayer;
