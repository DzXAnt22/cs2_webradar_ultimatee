import { useRef } from "react";
import { getRadarPosition, teamEnum, calculatePositionWithScale, getSmoothedTransitionMs } from "../utilities/utilities";

const Bomb = ({ bombData, mapData, radarImage, localTeam, averageLatency, settings }) => {
  const radarPosition = getRadarPosition(mapData, bombData);

  const bombRef = useRef();
  const bombBounding = (bombRef.current &&
    bombRef.current.getBoundingClientRect()) || { width: 0, height: 0 };

  const scaledPos = calculatePositionWithScale(radarImage, radarPosition);
    const radarImageTranslation = {
      x: (scaledPos[0] - bombBounding.width * 0.5),
      y: (scaledPos[1] - bombBounding.height * 0.5),
    };

  const baseSize = 1.5;
  const scaledSize = baseSize * (settings?.bombSize || 0.5);

  const transitionMs = getSmoothedTransitionMs(averageLatency, 0.55, 70, 160);

  return (
    <div
      className={`absolute origin-center rounded-[100%] left-0 top-0`}
      ref={bombRef}
      style={{
        width: `${scaledSize}vw`,
        height: `${scaledSize}vw`,
        transform: `translate(${radarImageTranslation.x}px, ${radarImageTranslation.y}px)`,
        transition: `transform ${transitionMs}ms linear`,
        backgroundColor: `${
          (bombData.m_is_defused && `#50904c`) ||
          (localTeam == teamEnum.counterTerrorist && `#6492b4`) ||
          `#c90b0b`
        }`,
        WebkitMask: `url('./assets/icons/c4_sml.png') no-repeat center / contain`,
        mask: `url('./assets/icons/c4_sml.png') no-repeat center / contain`,
        opacity: `${settings?.showOnlyEnemies && Number(bombData.owner_entity) < 429496729 && localTeam == teamEnum.terrorist ? 0 : 1}`,
        zIndex: `1`,
      }}
    />
  );
};

export default Bomb;