import { useRef } from "react";
import { calculatePositionWithScale, getRadarPosition, getSmoothedTransitionMs, teamEnum } from "../utilities/utilities";

const Bomb = ({ bombData, mapData, radarImage, localTeam, averageLatency, settings }) => {
  const bombRef = useRef();

  const bombX = Number(bombData?.x ?? bombData?.m_x);
  const bombY = Number(bombData?.y ?? bombData?.m_y);

  if (!Number.isFinite(bombX) || !Number.isFinite(bombY) || !radarImage) {
    return null;
  }

  const radarPosition = getRadarPosition(mapData, {
    x: bombX,
    y: bombY,
  });
  const bombBounding = (bombRef.current && bombRef.current.getBoundingClientRect()) || { width: 0, height: 0 };

  const scaledPos = calculatePositionWithScale(radarImage, radarPosition);
  const radarImageTranslation = {
    x: scaledPos[0] - bombBounding.width * 0.5,
    y: scaledPos[1] - bombBounding.height * 0.5,
  };

  const baseSize = 1.8;
  const configuredSize = Number(settings?.bombSize ?? 0.5);
  const scaledSize = Math.max(0.75, baseSize * configuredSize);

  const transitionMs = getSmoothedTransitionMs(averageLatency, 0.55, 70, 160);

  const iconGlowColor =
    (bombData?.m_is_defused && "rgba(80, 144, 76, 0.9)")
    || (bombData?.m_is_defusing && "rgba(100, 190, 255, 0.9)")
    || (localTeam === teamEnum.counterTerrorist ? "rgba(100, 146, 180, 0.9)" : "rgba(201, 11, 11, 0.9)");

  const shouldHideForSettings =
    settings?.showOnlyEnemies
    && Number(bombData?.owner_entity) < 429496729
    && localTeam === teamEnum.terrorist
    && !Number.isFinite(Number(bombData?.m_blow_time));

  return (
    <div
      className="absolute origin-center left-0 top-0"
      ref={bombRef}
      style={{
        width: `clamp(14px, ${scaledSize}vw, 36px)`,
        height: `clamp(14px, ${scaledSize}vw, 36px)`,
        transform: `translate(${radarImageTranslation.x}px, ${radarImageTranslation.y}px)`,
        transition: `transform ${transitionMs}ms linear`,
        opacity: shouldHideForSettings ? 0 : 1,
        zIndex: 58,
        pointerEvents: "none",
      }}
    >
      <div
        className="absolute inset-0 rounded-full"
        style={{
          boxShadow: `0 0 18px ${iconGlowColor}`,
          background: "radial-gradient(circle, rgba(12,26,44,0.55) 5%, rgba(6,13,22,0.08) 78%, rgba(0,0,0,0) 100%)",
        }}
      />

      <img
        src="./assets/icons/c4_sml.png"
        draggable={false}
        className="relative w-full h-full object-contain"
        style={{
          filter: `drop-shadow(0 0 10px ${iconGlowColor}) brightness(1.35) contrast(1.15)`,
        }}
      />
    </div>
  );
};

export default Bomb;
