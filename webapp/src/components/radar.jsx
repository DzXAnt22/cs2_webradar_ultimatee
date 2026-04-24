import { useRef } from "react";
import Draggable from "./Draggable";
import Player from "./player";
import Bomb from "./bomb";
import GrenadeLayer from "./GrenadeLayer";
import DroppedWeapon from "./droppedweapons";

const Radar = ({
  playerArray,
  radarImage,
  mapData,
  localTeam,
  averageLatency,
  bombData,
  settings,
  grenadeData,
  droppedWeaponsData,
  tempPlayer,
  radarZoom,
  radarScale,
}) => {
  const radarImageRef = useRef();
  const radarContentRef = useRef();

  return (
    <div id="radar" className="radar-card relative overflow-hidden origin-center" ref={radarContentRef}>
      <Draggable imgref={radarImageRef.current} avrPing={averageLatency} radarContentRef={radarContentRef.current}>
        <img
          onWheelCapture={radarZoom}
          ref={radarImageRef}
          className={`w-full h-auto rounded-[20px] ${
            bombData &&
            bombData.m_blow_time <= 10 &&
            tempPlayer &&
            !tempPlayer.m_is_dead &&
            tempPlayer.m_bomb_damage >= tempPlayer.m_health
              ? "alertAnim"
              : ""
          }`}
          src={radarImage}
          draggable={false}
          style={{
            scale: `${radarScale}`,
            transition: "scale 150ms linear",
            filter: `contrast(${200 - settings.mapBrightness || 100}%) brightness(${settings.mapBrightness || 100}%)`,
          }}
        />
      </Draggable>

      {playerArray &&
        playerArray.map((player) => (
          <Player
            key={player.m_idx}
            playerData={player}
            mapData={mapData}
            radarImage={radarImageRef.current}
            radarScale={radarScale}
            localTeam={localTeam}
            averageLatency={averageLatency}
            settings={settings}
            tempPlayer={tempPlayer}
          />
        ))}

      {bombData && (
        <Bomb
          bombData={bombData}
          mapData={mapData}
          radarImage={radarImageRef.current}
          localTeam={localTeam}
          averageLatency={averageLatency}
          settings={settings}
        />
      )}

      {settings.showGrenades && (
        <GrenadeLayer
          grenadeData={grenadeData}
          mapData={mapData}
          settings={settings}
          averageLatency={averageLatency}
          radarImage={radarImageRef.current}
        />
      )}

      {settings.showDroppedWeapons &&
        droppedWeaponsData &&
        droppedWeaponsData.map((droppedWeapon) => (
          <DroppedWeapon
            key={droppedWeapon.m_idx}
            droppedWeaponData={droppedWeapon}
            mapData={mapData}
            settings={settings}
            averageLatency={averageLatency}
            radarImage={radarImageRef.current}
          />
        ))}
    </div>
  );
};

export default Radar;
