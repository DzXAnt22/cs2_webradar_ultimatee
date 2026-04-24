import { useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import PlayerCard from "./components/playercard";
import Radar from "./components/radar";
import { getLatency, Latency } from "./components/latency";
import MaskedIcon from "./components/maskedicon";
import { colorSchemePallette } from "./utilities/utilities";
import { normalizeGrenadeType } from "./utilities/grenadeVisuals";

const CONNECTION_TIMEOUT = 5000;

/* change this to '1' if you want to use offline (your own pc only) */
const USE_LOCALHOST = 0;

/* you can get your public ip from https://ipinfo.io/ip */
const PUBLIC_IP = "PUBLIC_IP".trim();
const PORT = 22006;

let tempPlayer_ = null;
let languageData = {
  choosing_yourself: {
    main: "Select Yourself",
    explanation: "This is used for some features.",
    warning: "Please choose <b>YOURSELF</b>!",
  },
  settings: {
    button: "Settings",
    title: "Radar Settings",
    map_brightness: "Map Brightness",
    player_dot_size: "Player Dot Size",
    bomb_size: "Bomb Size",
    increase_player_contrast: "Increase Player Contrast",
    show_only_enemies: "Show Only Enemies",
    enemy_names: "Enemy Names",
    ally_names: "Ally Names",
    follow_yourself: "Follow Yourself",
    follow_yourself_rotation: "Follow Rotation",
    view_player_cones: "View Player Cones",
    show_grenades: "Show Grenades",
    show_grenades_color: "Grenade Color",
    show_greandes_size: "Grenade Size",
    show_dropped_weapons: "Show Dropped Weapons",
    show_dropped_weapons_lighter: "Use Lighter Color",
    show_dropped_weapons_ignore_grenades: "Ignore Grenades",
    show_dropped_weapons_size: "Weapon Size",
    language: "Language",
    theme_color_text: "Theme Color",
    theme_colors: {
      default: "Default",
      white: "White",
      light_blue: "Light Blue",
      dark_blue: "Dark Blue",
      purple: "Purple",
      red: "Red",
      orange: "Orange",
      yellow: "Yellow",
      green: "Green",
      light_green: "Light Green",
      pink: "Pink",
    },
    choose_yourself_again_button: "Choose Yourself Again",
  },
  bomb_timer: {
    lethal: "LETHAL",
  },
  radar_messages: {
    public_ip_not_set: ["A public IP address is required! Currently detected IP (", ") is a private/local IP"],
    websocket_connection_failed: [
      "WebSocket connection to '",
      "' failed. Please check the IP address and try again.",
    ],
    unsupported_map: "Current map is unsupported.",
    connected: "Connected! Please wait for the host to join match.",
  },
};

const EFFECTIVE_IP = USE_LOCALHOST
  ? "localhost"
  : PUBLIC_IP.match(/[a-zA-Z]/)
    ? window.location.hostname
    : PUBLIC_IP;

const DEFAULT_SETTINGS = {
  dotSize: 1,
  bombSize: 0.5,
  showAllNames: false,
  showEnemyNames: true,
  showViewCones: false,
  showOnlyEnemies: false,
  followYourself: false,
  followYourselfRotation: false,
  showDroppedWeapons: true,
  droppedWeaponSize: 0.5,
  droppedWeaponGlow: true,
  droppedWeaponIgnoreNade: false,
  showGrenades: true,
  thrownGrenadeSize: 0.5,
  thrownGrenadeColor: "#FF0000",
  grenadeEffectIntensity: 1,
  grenadeHighContrast: false,
  flashPulseEnabled: true,
  showGrenadeTimers: true,
  grenadePerformanceMode: false,
  whichPlayerAreYou: "0",
  mapBrightness: 100,
  increaseContrast: false,
  colorScheme: "default",
  language: "English",
  settings_version: "1.2",
};

const mergeSettings = (savedSettings) => ({
  ...DEFAULT_SETTINGS,
  ...(savedSettings || {}),
  settings_version: DEFAULT_SETTINGS.settings_version,
});

const pickFirstNumber = (...values) => {
  for (const value of values) {
    const numeric = Number(value);
    if (Number.isFinite(numeric)) {
      return numeric;
    }
  }

  return null;
};

const parseSocketPayload = async (payload) => {
  if (typeof payload === "string") {
    return JSON.parse(payload);
  }

  if (payload instanceof Blob) {
    return JSON.parse(await payload.text());
  }

  if (payload instanceof ArrayBuffer) {
    return JSON.parse(new TextDecoder().decode(payload));
  }

  if (ArrayBuffer.isView(payload)) {
    return JSON.parse(new TextDecoder().decode(payload));
  }

  return JSON.parse(String(payload));
};

const parseBooleanValue = (value) => {
  if (typeof value === "boolean") {
    return value;
  }

  if (typeof value === "number") {
    return value !== 0;
  }

  if (typeof value === "string") {
    const normalized = value.trim().toLowerCase();

    if (normalized === "true" || normalized === "1" || normalized === "yes") {
      return true;
    }

    if (normalized === "false" || normalized === "0" || normalized === "no" || normalized === "") {
      return false;
    }
  }

  return Boolean(value);
};

const normalizeGrenadeEntry = (grenade) => {
  if (!grenade || typeof grenade !== "object") {
    return null;
  }

  const grenadeType = grenade.m_type || grenade.type || grenade.name || grenade.grenade_type;
  const normalizedType = grenadeType ? normalizeGrenadeType(grenadeType) : "unknown";

  const x = pickFirstNumber(grenade.m_x, grenade.x, grenade.position?.x, grenade.pos?.x);
  const y = pickFirstNumber(grenade.m_y, grenade.y, grenade.position?.y, grenade.pos?.y);

  if (x == null || y == null) {
    return null;
  }

  const timeleft = pickFirstNumber(grenade.m_timeleft, grenade.timeleft, grenade.time_left, grenade.duration_left);
  const firePositions = Array.isArray(grenade.m_firePositions)
    ? grenade.m_firePositions
    : Array.isArray(grenade.firePositions)
      ? grenade.firePositions
      : null;

  return {
    ...grenade,
    m_idx: grenade.m_idx ?? grenade.idx ?? grenade.entity_idx,
    m_type: normalizedType,
    m_x: x,
    m_y: y,
    ...(timeleft != null ? { m_timeleft: timeleft } : {}),
    ...(firePositions ? { m_firePositions: firePositions } : {}),
  };
};

const isPersistentGrenade = (grenade) => {
  const normalizedType = normalizeGrenadeType(grenade.m_type);
  if (
    normalizedType === "smoke"
    || normalizedType === "smokegrenade"
    || normalizedType === "molo"
    || normalizedType === "molotov"
    || normalizedType === "incgrenade"
    || normalizedType === "inferno"
  ) {
    return true;
  }

  return Number.isFinite(grenade.m_timeleft) && grenade.m_timeleft > 0;
};

const normalizeGrenadePayload = (payload) => {
  const normalized = {
    landed: [],
    thrown: [],
  };

  if (!payload) {
    return normalized;
  }

  if (Array.isArray(payload)) {
    payload.forEach((grenade) => {
      const normalizedGrenade = normalizeGrenadeEntry(grenade);
      if (!normalizedGrenade) {
        return;
      }

      if (isPersistentGrenade(normalizedGrenade)) {
        normalized.landed.push(normalizedGrenade);
      } else {
        normalized.thrown.push(normalizedGrenade);
      }
    });

    return normalized;
  }

  const landedSource = payload.landed || payload.m_landed || payload.active || payload.smokes || payload.infernos || [];
  const thrownSource = payload.thrown || payload.m_thrown || payload.projectiles || payload.airborne || [];

  if (Array.isArray(landedSource)) {
    normalized.landed = landedSource.map(normalizeGrenadeEntry).filter(Boolean);
  }

  if (Array.isArray(thrownSource)) {
    normalized.thrown = thrownSource.map(normalizeGrenadeEntry).filter(Boolean);
  }

  if (normalized.landed.length === 0 && normalized.thrown.length === 0) {
    Object.values(payload).forEach((grenadeCandidate) => {
      const normalizedGrenade = normalizeGrenadeEntry(grenadeCandidate);
      if (!normalizedGrenade) {
        return;
      }

      if (isPersistentGrenade(normalizedGrenade)) {
        normalized.landed.push(normalizedGrenade);
      } else {
        normalized.thrown.push(normalizedGrenade);
      }
    });
  }

  return normalized;
};

const normalizeBombPayload = (payload) => {
  if (!payload || typeof payload !== "object") {
    return undefined;
  }

  const x = pickFirstNumber(payload.x, payload.m_x, payload.bomb_x, payload.position?.x, payload.m_position?.x);
  const y = pickFirstNumber(payload.y, payload.m_y, payload.bomb_y, payload.position?.y, payload.m_position?.y);
  const blowTime = pickFirstNumber(payload.m_blow_time, payload.blow_time, payload.bomb_time);
  const defuseTime = pickFirstNumber(payload.m_defuse_time, payload.defuse_time);

  if (x == null || y == null) {
    return undefined;
  }

  return {
    ...payload,
    x,
    y,
    ...(blowTime != null ? { m_blow_time: blowTime } : {}),
    ...(defuseTime != null ? { m_defuse_time: defuseTime } : {}),
    m_is_defused: parseBooleanValue(payload.m_is_defused ?? payload.is_defused),
    m_is_defusing: parseBooleanValue(payload.m_is_defusing ?? payload.is_defusing),
  };
};

const loadLanguageFile = async (language) => {
  await fetch(`/lang/${language}.json`)
    .then((res) => res.text())
    .then((text) => {
      languageData = JSON.parse(text);
    });
};

const loadSettings = () => {
  let parsedSettings = null;

  try {
    parsedSettings = JSON.parse(localStorage.getItem("radarSettings"));
  } catch {
    parsedSettings = null;
  }

  const mergedSettings = mergeSettings(parsedSettings);
  loadLanguageFile(mergedSettings.language);
  return mergedSettings;
};

const LanguageSelectionModal = ({ languages, onSelect }) => (
  <div className="fixed inset-0 bg-black/65 backdrop-blur-md flex justify-center items-center z-[100]">
    <div className="glass-panel rounded-2xl p-7 w-96 max-w-[90vw] border border-white/20">
      <h2 className="text-xl font-bold text-white mb-4">Select Language</h2>
      <ul className="max-h-60 overflow-y-auto pr-2 space-y-2">
        {languages.map((lang) => (
          <li
            key={lang}
            onClick={() => onSelect(lang)}
            className="p-3 glass-panel-soft hover:bg-radar-blue/20 text-white rounded-md cursor-pointer transition-all duration-200"
          >
            <span className="font-medium truncate">{lang}</span>
          </li>
        ))}
      </ul>
    </div>
  </div>
);

const PlayerSelectionModal = ({ players, onSelect, localTeam, translation }) => (
  <div className="fixed inset-0 bg-black/65 backdrop-blur-md flex justify-center items-center z-[100]">
    <div className="glass-panel rounded-2xl p-7 w-96 max-w-[90vw] border border-white/20">
      <h2 className="text-xl font-bold text-white mb-4">{translation.choosing_yourself.main}</h2>
      <p className="text-sm text-gray-300 mb-6">
        {translation.choosing_yourself.explanation}
        <br />
        <a dangerouslySetInnerHTML={{ __html: translation.choosing_yourself.warning }} />
      </p>
      <ul className="max-h-60 overflow-y-auto pr-2 space-y-2">
        {players
          .filter((player) => player.m_steam_id !== "0")
          .filter((player) => player.m_team === localTeam)
          .map((player) => (
            <li
              key={player.m_steam_id}
              onClick={() => onSelect(player.m_steam_id)}
              className="p-3 glass-panel-soft hover:bg-radar-blue/20 text-white rounded-md cursor-pointer transition-all duration-200 flex justify-between items-center"
            >
              <span className="font-medium truncate">{player.m_name}</span>
              <span className="text-xs px-2 py-0.5 rounded-full">{player.m_steam_id}</span>
            </li>
          ))}
      </ul>
    </div>
  </div>
);

const App = () => {
  const [averageLatency, setAverageLatency] = useState(0);
  const [playerArray, setPlayerArray] = useState([]);
  const [grenadeData, setGrenadeData] = useState({ landed: [], thrown: [] });
  const [droppedWeaponsData, setDroppedWeaponsData] = useState([]);
  const [mapData, setMapData] = useState();
  const [localTeam, setLocalTeam] = useState();
  const [bombData, setBombData] = useState();
  const [settings, setSettings] = useState(loadSettings());
  const [translation, updateTranslation] = useState(languageData);
  const [showPlayerPrompt, setShowPlayerPrompt] = useState(false);
  const [showLangPrompt, setShowLangPrompt] = useState(false);
  const [radarScale, setRadarScale] = useState(1);
  const mapMetaCache = useRef({});

  const radarZoom = (event) => {
    const delta = event.deltaY * -0.001;
    const newScale = radarScale + delta;
    if (newScale > 0.3 && newScale < 4) {
      setRadarScale(newScale);
    }
  };

  const langFiles = import.meta.glob("/public/lang/*.json", {
    query: "?url",
    import: "default",
  });

  const languageOptions = useMemo(
    () =>
      Object.keys(langFiles).map((path) => {
        const fileName = path.split("/").pop();
        return fileName.replace(".json", "");
      }),
    []
  );

  useEffect(() => {
    localStorage.setItem("radarSettings", JSON.stringify(settings));
  }, [settings]);

  useEffect(() => {
    loadLanguageFile(settings.language).then(() => {
      updateTranslation(languageData);
    });
  }, [settings.language]);

  useEffect(() => {
    if (
      (settings.whichPlayerAreYou === "0" && playerArray.length > 0) ||
      (settings.whichPlayerAreYou === undefined && playerArray.length > 0)
    ) {
      setShowLangPrompt(true);
    } else if (settings.whichPlayerAreYou !== "0" && settings.whichPlayerAreYou !== undefined) {
      setShowLangPrompt(false);
    }
  }, [playerArray, settings.whichPlayerAreYou]);

  const handlePlayerSelect = (playerIdx) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      whichPlayerAreYou: playerIdx,
    }));
    setShowPlayerPrompt(false);
  };

  const handleLangSelect = (lang) => {
    setSettings((prevSettings) => ({
      ...prevSettings,
      language: lang,
    }));
    setShowLangPrompt(false);
    setShowPlayerPrompt(true);
  };

  useEffect(() => {
    let webSocket = null;
    let connectionTimeout = null;

    const fetchData = async () => {
      let webSocketURL = null;

      if (PUBLIC_IP.startsWith("192.168")) {
        document.getElementsByClassName(
          "radar_message"
        )[0].textContent = `${translation.radar_messages.public_ip_not_set[0]}${PUBLIC_IP}${translation.radar_messages.public_ip_not_set[1]}`;
        return;
      }

      try {
        webSocketURL = USE_LOCALHOST
          ? `ws://localhost:${PORT}/cs2_webradar`
          : `ws://${EFFECTIVE_IP}:${PORT}/cs2_webradar`;
        webSocket = new WebSocket(webSocketURL);
      } catch (error) {
        document.getElementsByClassName("radar_message")[0].textContent = `${error}`;
        return;
      }

      connectionTimeout = setTimeout(() => {
        webSocket.close();
      }, CONNECTION_TIMEOUT);

      webSocket.onopen = async () => {
        clearTimeout(connectionTimeout);
        console.info("connected to the web socket");
      };

      webSocket.onclose = async () => {
        clearTimeout(connectionTimeout);
        console.error("disconnected from the web socket");
      };

      webSocket.onerror = async (error) => {
        clearTimeout(connectionTimeout);
        document.getElementsByClassName(
          "radar_message"
        )[0].textContent = `${translation.radar_messages.websocket_connection_failed[0]}${webSocketURL}${translation.radar_messages.websocket_connection_failed[1]}`;
        console.error(error);
      };

      webSocket.onmessage = async (event) => {
        setAverageLatency(getLatency());

        let parsedData = null;

        try {
          parsedData = await parseSocketPayload(event.data);
        } catch (error) {
          console.error("Failed to parse websocket payload", error);
          return;
        }

        setLocalTeam((previousTeam) => parsedData.m_local_team ?? parsedData.local_team ?? previousTeam);
        setBombData(normalizeBombPayload(parsedData.m_bomb ?? parsedData.bomb ?? parsedData.m_bomb_data));
        setGrenadeData(
          normalizeGrenadePayload(parsedData.m_grenades ?? parsedData.grenades ?? parsedData.grenade_data)
        );
        setDroppedWeaponsData(
          Array.isArray(parsedData.m_dropped_weapons)
            ? parsedData.m_dropped_weapons
            : Array.isArray(parsedData.dropped_weapons)
              ? parsedData.dropped_weapons
              : []
        );

        const map = parsedData.m_map ?? parsedData.map;

        if (!map) {
          return;
        }

        if (map === "invalid") {
          setMapData({ name: "invalid" });
          setPlayerArray([]);
          document.body.style.backgroundImage = "url(./data/de_mirage/background.png)";
          return;
        }

        if (!mapMetaCache.current[map]) {
          const response = await fetch(`data/${map}/data.json`);
          mapMetaCache.current[map] = response.status === 200 ? await response.json() : null;
        }

        if (mapMetaCache.current[map]) {
          setPlayerArray(Array.isArray(parsedData.m_players) ? parsedData.m_players : parsedData.players || []);
          setMapData({
            ...mapMetaCache.current[map],
            name: map,
          });
          document.body.style.backgroundImage = `url(./data/${map}/background.png)`;
          return;
        }

        setMapData({ name: "unsupported" });
        setPlayerArray([]);
        document.body.style.backgroundImage = "url(./data/de_mirage/background.png)";
      };
    };

    fetchData();

    return () => {
      if (connectionTimeout) {
        clearTimeout(connectionTimeout);
      }

      if (webSocket && webSocket.readyState < 2) {
        webSocket.close();
      }
    };
  }, []);

  useEffect(() => {
    document.body.style.overflow = "hidden";
  }, []);

  if (playerArray && playerArray.length > 0) {
    tempPlayer_ = playerArray.find((player) => player.m_steam_id === settings.whichPlayerAreYou);
  }

  const showRadar =
    playerArray &&
    playerArray.length > 0 &&
    mapData &&
    mapData.name !== "invalid" &&
    mapData.name !== "unsupported" &&
    settings.whichPlayerAreYou &&
    settings.whichPlayerAreYou !== "0";

  const radarFrameSizeClass = "w-[min(84vh,84vw)] min-w-[22rem] max-w-[68rem]";
  const bombBlowTime = pickFirstNumber(bombData?.m_blow_time, bombData?.blow_time) ?? 0;
  const bombDefuseTime = pickFirstNumber(bombData?.m_defuse_time, bombData?.defuse_time) ?? 0;
  const bombIsDefused = Boolean(bombData?.m_is_defused);
  const bombIsDefusing = Boolean(bombData?.m_is_defusing);

  return (
    <div className="radar-app-shell">
      {showLangPrompt && <LanguageSelectionModal languages={languageOptions} onSelect={handleLangSelect} />}
      {showPlayerPrompt && playerArray.length > 0 && (
        <PlayerSelectionModal
          players={playerArray}
          onSelect={handlePlayerSelect}
          localTeam={localTeam}
          translation={translation}
        />
      )}

      <div className="w-full h-full relative flex flex-col overflow-hidden">
        <Latency
          value={averageLatency}
          settings={settings}
          setSettings={setSettings}
          translation={translation}
          languages={languageOptions}
        />

        {bombData && bombBlowTime > 0 && !bombIsDefused && (
          <div className="absolute left-1/2 -translate-x-1/2 top-4 z-40">
            <div className="glass-panel rounded-2xl px-4 py-2 flex flex-col items-center gap-1">
              <div className="flex justify-center items-center gap-1">
                <MaskedIcon
                  path="./assets/icons/c4_sml.png"
                  height={32}
                  color={
                    (bombIsDefusing && bombBlowTime - bombDefuseTime > 0 && "#00FF00") ||
                    (bombBlowTime - bombDefuseTime < 0 && "#FF0000") ||
                    `${colorSchemePallette[settings.colorScheme][1]}`
                  }
                />
                <span className="font-semibold">{`${bombBlowTime.toFixed(1)}s ${
                  (bombIsDefusing && `(${bombDefuseTime.toFixed(1)}s)`) || ""
                }`}</span>
              </div>

              {tempPlayer_ && !tempPlayer_.m_is_dead && (
                <div
                  className="flex justify-center text-sm"
                  style={{
                    color: (() => {
                      const playerHealth = Math.max(Number(tempPlayer_.m_health) || 0, 1);
                      const playerBombDamage = Number(tempPlayer_.m_bomb_damage) || 0;
                      const ratio = playerBombDamage / playerHealth;

                      if (ratio >= 1.0) {
                        return "rgb(255, 0, 0)";
                      }

                      if (ratio >= 0.5) {
                        const decimal = (ratio - 0.5) * 2;
                        return `rgb(255, ${Math.floor(255 * (1 - decimal))}, 0)`;
                      }

                      const decimal = ratio * 2;
                      return `rgb(${Math.floor(255 * decimal)}, 255, 0)`;
                    })(),
                    fontWeight:
                      (Number(tempPlayer_.m_bomb_damage) || 0) / Math.max(Number(tempPlayer_.m_health) || 0, 1) >= 1
                        ? "bold"
                        : "normal",
                  }}
                >
                  {`${
                    (Number(tempPlayer_.m_bomb_damage) || 0) < (Number(tempPlayer_.m_health) || 0)
                      ? (Number(tempPlayer_.m_bomb_damage) || 0) < 7
                        ? "0 HP"
                        : `-${Number(tempPlayer_.m_bomb_damage) || 0} HP`
                      : `⚠️ ${translation.bomb_timer.lethal} ⚠️`
                  }`}
                </div>
              )}
            </div>
          </div>
        )}

        <div className="flex-1 flex items-center justify-center px-2 xl:px-4 pt-20 pb-3 gap-2 xl:gap-3">
          <ul className="xl:flex hidden flex-col gap-2 m-0 p-2 w-[20rem] max-h-[88vh] overflow-y-auto glass-panel-soft rounded-3xl">
            {playerArray &&
              playerArray
                .filter((player) => player.m_team === 2)
                .map((player) => (
                  <PlayerCard
                    isOnRightSide={false}
                    key={player.m_idx}
                    playerData={player}
                    settings={settings}
                  />
                ))}
          </ul>

          {showRadar ? (
            <div className="flex flex-col items-center gap-3">
              <div className={radarFrameSizeClass}>
                <Radar
                  playerArray={playerArray}
                  radarImage={
                    tempPlayer_ && mapData.leveling && tempPlayer_.m_position.z < mapData.level_change
                      ? `./data/${mapData.name}/radar_lower.png`
                      : `./data/${mapData.name}/radar.png`
                  }
                  mapData={mapData}
                  localTeam={localTeam}
                  averageLatency={averageLatency}
                  bombData={bombData}
                  settings={settings}
                  grenadeData={grenadeData}
                  droppedWeaponsData={droppedWeaponsData}
                  tempPlayer={tempPlayer_}
                  radarZoom={radarZoom}
                  radarScale={radarScale}
                />
              </div>

              <div className={`glass-panel rounded-2xl px-4 py-3 ${radarFrameSizeClass} flex items-center gap-3`}>
                <span className="text-xs uppercase tracking-[0.1em] text-radar-secondary">Zoom</span>
                <input
                  type="range"
                  min="0.3"
                  max="4"
                  step="0.1"
                  value={radarScale || 1}
                  onChange={(event) => {
                    setRadarScale(parseFloat(event.target.value));
                  }}
                  className="relative flex-1 h-2 rounded-lg appearance-none cursor-pointer accent-radar-primary"
                  style={{
                    background: `linear-gradient(to right, #b1d0e7 ${(((radarScale || 1) - 0.3) / 3.7) * 100}%, rgba(59, 130, 246, 0.2) ${(((radarScale || 1) - 0.3) / 3.7) * 100}%)`,
                  }}
                />
                <span className="text-xs font-mono text-radar-primary min-w-[3.2rem] text-right">
                  {(radarScale || 1).toFixed(1)}x
                </span>
              </div>
            </div>
          ) : mapData && mapData.name === "unsupported" ? (
            <div id="radar" className={`radar-card glass-panel-soft ${radarFrameSizeClass}`}>
              <h1 className="radar_message">{translation.radar_messages.unsupported_map}</h1>
            </div>
          ) : (
            <div id="radar" className={`radar-card glass-panel-soft ${radarFrameSizeClass}`}>
              <h1 className="radar_message">{translation.radar_messages.connected}</h1>
            </div>
          )}

          <ul className="xl:flex hidden flex-col gap-2 m-0 p-2 w-[20rem] max-h-[88vh] overflow-y-auto glass-panel-soft rounded-3xl">
            {playerArray &&
              playerArray
                .filter((player) => player.m_team === 3)
                .map((player) => (
                  <PlayerCard
                    isOnRightSide
                    key={player.m_idx}
                    playerData={player}
                    settings={settings}
                  />
                ))}
          </ul>
        </div>
      </div>
    </div>
  );
};

export default App;
