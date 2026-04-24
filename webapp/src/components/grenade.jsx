import GrenadeEffects from "./grenadeeffects";
import MaskedIcon from "./maskedicon";
import { calculatePositionWithScale, getRadarPosition } from "../utilities/utilities";
import {
  getGrenadeEffectIntensity,
  getGrenadeIconName,
  getGrenadeTransitionMs,
  getThrownGrenadeStyle,
  GRENADE_RENDER_STATE,
  normalizeGrenadeType,
} from "../utilities/grenadeVisuals";

const Grenade = ({ grenadeData, mapData, settings, averageLatency, radarImage, renderState }) => {
  const normalizedType = normalizeGrenadeType(grenadeData.m_type);

  if (renderState !== GRENADE_RENDER_STATE.THROWN) {
    return (
      <GrenadeEffects
        grenadeData={{ ...grenadeData, m_type: normalizedType }}
        renderState={renderState}
        mapData={mapData}
        settings={settings}
        averageLatency={averageLatency}
        radarImage={radarImage}
      />
    );
  }

  if (!radarImage) {
    return null;
  }

  const radarPosition = getRadarPosition(mapData, {
    x: grenadeData.m_x,
    y: grenadeData.m_y,
  });

  const [x, y] = calculatePositionWithScale(radarImage, radarPosition);
  const transitionMs = getGrenadeTransitionMs(averageLatency);
  const intensity = getGrenadeEffectIntensity(settings);
  const styleToken = getThrownGrenadeStyle(normalizedType, settings);
  const iconName = getGrenadeIconName(normalizedType);
  const isIncendiary = normalizedType === "molotov" || normalizedType === "incgrenade";

  return (
    <div
      className="absolute grenade-thrown-shell left-0 top-0"
      style={{
        transform: `translate(${x}px, ${y}px) translate(-50%, -50%)`,
        transition: `transform ${transitionMs}ms cubic-bezier(0.22, 1, 0.36, 1)`,
        zIndex: styleToken.zIndex,
      }}
    >
      <div
        className="grenade-thrown-ring"
        style={{
          color: styleToken.ringColor,
          opacity: isIncendiary ? 0.95 : 0.75,
        }}
      />

      <div
        style={{
          filter: `drop-shadow(${styleToken.coreGlow})`,
        }}
      >
        <MaskedIcon
          path={`./assets/icons/${iconName}.svg`}
          height={`${(settings.thrownGrenadeSize || 0.5) * intensity}vw`}
          color={styleToken.iconColor}
        />
      </div>
    </div>
  );
};

export default Grenade;
