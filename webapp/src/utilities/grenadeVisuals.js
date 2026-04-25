import { clampNumber, getSmoothedTransitionMs } from "./utilities.jsx";

export const GRENADE_RENDER_STATE = Object.freeze({
  THROWN: "thrown",
  BURNING: "burning",
  SMOKE_ACTIVE: "smoke_active",
  FLASH_PULSE: "flash_pulse",
  HE_PULSE: "he_pulse",
});

const TYPE_ALIASES = {
  smoke: "smokegrenade",
  smokegrenade: "smokegrenade",
  smoke_grenade: "smokegrenade",
  flash: "flashbang",
  flashbang: "flashbang",
  he: "hegrenade",
  hegrenade: "hegrenade",
  frag: "hegrenade",
  fraggrenade: "hegrenade",
  decoy: "decoy",
  molo: "molo",
  molotov: "molotov",
  incgrenade: "incgrenade",
  incendiary: "incgrenade",
  inferno: "molo",
};

const THROWN_COLORS = {
  smokegrenade: "#9CA3AF",
  flashbang: "#FFE58A",
  hegrenade: "#F97373",
  decoy: "#67E8F9",
  molotov: "#FF8A33",
  molo: "#FF8A33",
  incgrenade: "#FF8A33",
  default: "#F2F7FF",
};

export const normalizeGrenadeType = (type) => {
  if (!type) {
    return "unknown";
  }

  const normalized = String(type).trim().toLowerCase();
  return TYPE_ALIASES[normalized] || normalized;
};

export const isBurningGrenadeType = (type) => {
  const normalizedType = normalizeGrenadeType(type);
  return (
    normalizedType === "molo"
    || normalizedType === "molotov"
    || normalizedType === "incgrenade"
    || normalizedType === "inferno"
  );
};

export const isSmokeGrenadeType = (type) => normalizeGrenadeType(type) === "smokegrenade";

export const isHeGrenadeType = (type) => normalizeGrenadeType(type) === "hegrenade";

export const buildGrenadeKey = (grenade, channel = "main") => {
  const normalizedType = normalizeGrenadeType(grenade?.m_type);
  const mIdx = grenade?.m_idx;

  if (mIdx !== undefined && mIdx !== null && `${mIdx}` !== "") {
    return `${channel}:${mIdx}:${normalizedType}`;
  }

  const approxX = Number.isFinite(Number(grenade?.m_x)) ? Number(grenade.m_x).toFixed(1) : "x";
  const approxY = Number.isFinite(Number(grenade?.m_y)) ? Number(grenade.m_y).toFixed(1) : "y";
  return `${channel}:no_idx:${normalizedType}:${approxX}:${approxY}`;
};

export const getGrenadeEffectIntensity = (settings) =>
  clampNumber(Number(settings?.grenadeEffectIntensity ?? 1), 0.4, 1.8);

export const getGrenadeTransitionMs = (latency) => getSmoothedTransitionMs(latency, 0.55, 72, 165);

export const getThrownPersistMs = (type, settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  const normalized = normalizeGrenadeType(type);
  const base = normalized === "flashbang" ? 180 : 260;
  const performancePenalty = settings?.grenadePerformanceMode ? -60 : 0;
  return clampNumber(Math.round(base + (intensity - 1) * 80 + performancePenalty), 120, 420);
};

export const getLandedPersistMs = (type, settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  const normalized = normalizeGrenadeType(type);
  const isBurning = normalized === "molo" || normalized === "molotov" || normalized === "incgrenade";
  const base = isBurning ? 350 : 260;
  const performancePenalty = settings?.grenadePerformanceMode ? -50 : 0;
  return clampNumber(Math.round(base + (intensity - 1) * 80 + performancePenalty), 170, 560);
};

export const getFlashPulseDurationMs = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  return clampNumber(Math.round(300 + (intensity - 1) * 140), 250, 450);
};

export const getHePulseDurationMs = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  return clampNumber(Math.round(420 + (intensity - 1) * 180), 320, 680);
};

export const getPulseMissingGraceMs = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  const performancePenalty = settings?.grenadePerformanceMode ? 35 : 0;
  return clampNumber(Math.round(125 + (1 - intensity) * 30 + performancePenalty), 80, 220);
};

export const getThrownGrenadeStyle = (grenadeType, settings) => {
  const normalizedType = normalizeGrenadeType(grenadeType);
  const highContrast = Boolean(settings?.grenadeHighContrast);

  const defaultColor = settings?.thrownGrenadeColor || THROWN_COLORS.default;
  const iconColor = THROWN_COLORS[normalizedType] || defaultColor;

  return {
    iconColor,
    ringColor: highContrast ? "#FFFFFF" : iconColor,
    coreGlow: highContrast ? "0 0 24px rgba(255,255,255,0.9)" : `0 0 16px ${iconColor}`,
    zIndex: isBurningGrenadeType(normalizedType) ? 52 : 48,
  };
};

const KNOWN_GRENADE_ICONS = new Set([
  "smokegrenade",
  "flashbang",
  "hegrenade",
  "decoy",
  "molotov",
  "molo",
  "incgrenade",
  "inferno",
]);

export const getGrenadeIconName = (grenadeType) => {
  const normalizedType = normalizeGrenadeType(grenadeType);
  if (normalizedType === "molo") {
    return "inferno";
  }

  return KNOWN_GRENADE_ICONS.has(normalizedType) ? normalizedType : "decoy";
};
