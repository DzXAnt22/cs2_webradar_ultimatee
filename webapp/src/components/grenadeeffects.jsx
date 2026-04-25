import { calculatePositionWithScale, getRadarPosition } from "../utilities/utilities";
import { GRENADE_RENDER_STATE, getGrenadeEffectIntensity, getGrenadeTransitionMs } from "../utilities/grenadeVisuals";

const TimerChip = ({ value, visible }) => {
  const numericValue = Number(value);

  if (!visible || !Number.isFinite(numericValue) || numericValue < 0) {
    return null;
  }

  return <div className="grenade-timer-chip">{numericValue.toFixed(1)}s</div>;
};

const getNodeScale = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  return {
    smoke: 3.8 * intensity,
    fireNode: 1.35 * intensity,
    flash: 3.6 * intensity,
    hePulse: 4.2 * intensity,
  };
};

const getFirePositions = (grenadeData, settings) => {
  const positions = Array.isArray(grenadeData.m_firePositions) ? grenadeData.m_firePositions : [];
  const fallback = [[grenadeData.m_x, grenadeData.m_y]];
  const source = positions.length > 0 ? positions : fallback;

  const normalizedPositions = source
    .map((point) => {
      if (Array.isArray(point) && point.length >= 2) {
        return [Number(point[0]), Number(point[1])];
      }

      if (point && typeof point === "object") {
        return [Number(point.x ?? point.m_x), Number(point.y ?? point.m_y)];
      }

      return null;
    })
    .filter((point) => point && Number.isFinite(point[0]) && Number.isFinite(point[1]));

  if (!settings?.grenadePerformanceMode) {
    return normalizedPositions.slice(0, 64);
  }

  return normalizedPositions.filter((_, index) => index % 2 === 0).slice(0, 24);
};

const GrenadeEffects = ({ grenadeData, renderState, mapData, settings, averageLatency, radarImage }) => {
  const scales = getNodeScale(settings);
  const transitionMs = getGrenadeTransitionMs(averageLatency);
  const showTimers = settings?.showGrenadeTimers !== false;
  const highContrast = Boolean(settings?.grenadeHighContrast);

  if (!radarImage) {
    return null;
  }

  const centerRadarPosition = getRadarPosition(mapData, {
    x: grenadeData.m_x,
    y: grenadeData.m_y,
  });

  const [centerX, centerY] = calculatePositionWithScale(radarImage, centerRadarPosition);

  if (renderState === GRENADE_RENDER_STATE.SMOKE_ACTIVE) {
    return (
      <div
        className="absolute grenade-smoke-shell"
        style={{
          width: `${scales.smoke}vw`,
          height: `${scales.smoke}vw`,
          transform: `translate(${centerX}px, ${centerY}px) translate(-50%, -50%)`,
          transition: `transform ${transitionMs}ms cubic-bezier(0.22, 1, 0.36, 1)`,
          zIndex: 64,
          pointerEvents: "none",
        }}
      >
        <div
          className="grenade-smoke-outer"
          style={{
            backgroundColor: highContrast ? "rgba(250, 250, 250, 0.62)" : "rgba(203, 213, 225, 0.52)",
            border: `1px solid ${highContrast ? "rgba(255, 255, 255, 0.85)" : "rgba(226, 232, 240, 0.68)"}`,
            filter: `blur(${settings?.grenadePerformanceMode ? 2 : 5}px)`,
          }}
        />
        <div
          className="grenade-smoke-inner"
          style={{
            inset: "14%",
            backgroundColor: highContrast ? "rgba(255,255,255,0.5)" : "rgba(226, 232, 240, 0.38)",
            filter: `blur(${settings?.grenadePerformanceMode ? 1 : 4}px)`,
          }}
        />
        <TimerChip value={grenadeData.m_timeleft} visible={showTimers} />
      </div>
    );
  }

  if (renderState === GRENADE_RENDER_STATE.BURNING) {
    return (
      <>
        {getFirePositions(grenadeData, settings).map((firePosition, index) => {
          const fireRadarPosition = getRadarPosition(mapData, {
            x: firePosition[0],
            y: firePosition[1],
          });
          const [fireX, fireY] = calculatePositionWithScale(radarImage, fireRadarPosition);

          return (
            <div
              key={`${grenadeData.cacheKey || grenadeData.m_idx}:fire:${index}`}
              className="absolute grenade-fire-node"
              style={{
                width: `${scales.fireNode}vw`,
                height: `${scales.fireNode}vw`,
                transform: `translate(${fireX}px, ${fireY}px) translate(-50%, -50%)`,
                transition: `transform ${transitionMs}ms cubic-bezier(0.22, 1, 0.36, 1)`,
                zIndex: 70,
                pointerEvents: "none",
              }}
            >
              <div
                className="grenade-fire-core"
                style={{
                  background:
                    "radial-gradient(circle, rgba(255,248,188,0.9) 8%, rgba(255,133,32,0.92) 45%, rgba(255,67,20,0.62) 100%)",
                  boxShadow: highContrast
                    ? "0 0 22px rgba(255, 255, 255, 0.85)"
                    : "0 0 20px rgba(255, 87, 34, 0.78)",
                }}
              />
            </div>
          );
        })}

        <div
          className="absolute"
          style={{
            transform: `translate(${centerX}px, ${centerY}px) translate(-50%, -50%)`,
            transition: `transform ${transitionMs}ms cubic-bezier(0.22, 1, 0.36, 1)`,
            zIndex: 74,
            pointerEvents: "none",
          }}
        >
          <TimerChip value={grenadeData.m_timeleft} visible={showTimers} />
        </div>
      </>
    );
  }

  if (renderState === GRENADE_RENDER_STATE.FLASH_PULSE) {
    return (
      <div
        className="absolute grenade-flash-shell"
        style={{
          width: `${scales.flash}vw`,
          height: `${scales.flash}vw`,
          transform: `translate(${centerX}px, ${centerY}px) translate(-50%, -50%)`,
          zIndex: 82,
          pointerEvents: "none",
        }}
      >
        <div
          className="grenade-flash-core"
          style={{
            background: "radial-gradient(circle, rgba(255,255,255,0.95) 12%, rgba(255,245,182,0.7) 42%, rgba(255,255,255,0) 100%)",
            boxShadow: highContrast
              ? "0 0 40px rgba(255, 255, 255, 0.95)"
              : "0 0 28px rgba(255, 240, 180, 0.7)",
          }}
        />
      </div>
    );
  }

  if (renderState === GRENADE_RENDER_STATE.HE_PULSE) {
    return (
      <div
        className="absolute grenade-he-shell"
        style={{
          width: `${scales.hePulse}vw`,
          height: `${scales.hePulse}vw`,
          transform: `translate(${centerX}px, ${centerY}px) translate(-50%, -50%)`,
          zIndex: 80,
          pointerEvents: "none",
        }}
      >
        <div
          className="grenade-he-core"
          style={{
            background:
              "radial-gradient(circle, rgba(255, 248, 220, 0.92) 0%, rgba(255, 140, 66, 0.72) 40%, rgba(255, 72, 0, 0.34) 66%, rgba(255, 72, 0, 0) 100%)",
            boxShadow: highContrast
              ? "0 0 34px rgba(255, 255, 255, 0.9)"
              : "0 0 26px rgba(255, 110, 64, 0.85)",
          }}
        />
        <div
          className="grenade-he-ring"
          style={{
            borderColor: highContrast ? "rgba(255,255,255,0.92)" : "rgba(255, 122, 69, 0.9)",
          }}
        />
      </div>
    );
  }

  return null;
};

export default GrenadeEffects;
