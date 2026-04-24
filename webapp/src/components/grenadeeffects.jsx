import { calculatePositionWithScale, getRadarPosition } from "../utilities/utilities";
import { GRENADE_RENDER_STATE, getGrenadeEffectIntensity, getGrenadeTransitionMs } from "../utilities/grenadeVisuals";

const TimerChip = ({ value, visible }) => {
  if (!visible || value == null) {
    return null;
  }

  return <div className="grenade-timer-chip">{value.toFixed(1)}s</div>;
};

const getNodeScale = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  return {
    smoke: 3.2 * intensity,
    fireNode: 1.2 * intensity,
    flash: 3.6 * intensity,
  };
};

const getFirePositions = (grenadeData, settings) => {
  const positions = Array.isArray(grenadeData.m_firePositions) ? grenadeData.m_firePositions : [];
  const fallback = [[grenadeData.m_x, grenadeData.m_y]];
  const source = positions.length > 0 ? positions : fallback;

  if (!settings?.grenadePerformanceMode) {
    return source.slice(0, 64);
  }

  return source.filter((_, index) => index % 2 === 0).slice(0, 24);
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
          zIndex: 26,
          pointerEvents: "none",
        }}
      >
        <div
          className="grenade-smoke-outer"
          style={{
            backgroundColor: highContrast ? "rgba(248, 250, 252, 0.35)" : "rgba(148, 163, 184, 0.35)",
            filter: `blur(${settings?.grenadePerformanceMode ? 2 : 5}px)`,
          }}
        />
        <div
          className="grenade-smoke-inner"
          style={{
            inset: "12%",
            backgroundColor: highContrast ? "rgba(255,255,255,0.28)" : "rgba(203, 213, 225, 0.24)",
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
                zIndex: 30,
                pointerEvents: "none",
              }}
            >
              <div
                className="grenade-fire-core"
                style={{
                  background:
                    "radial-gradient(circle, rgba(255,248,188,0.88) 8%, rgba(255,133,32,0.85) 45%, rgba(255,67,20,0.55) 100%)",
                  boxShadow: highContrast
                    ? "0 0 20px rgba(255, 255, 255, 0.8)"
                    : "0 0 18px rgba(255, 87, 34, 0.65)",
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
            zIndex: 32,
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
          zIndex: 52,
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

  return null;
};

export default GrenadeEffects;
