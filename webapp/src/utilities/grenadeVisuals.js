import { clampNumber, getSmoothedTransitionMs } from "./utilities.jsx";

export const GRENADE_RENDER_STATE = Object.freeze({
  THROWN: "thrown",
  BURNING: "burning",
  SMOKE_ACTIVE: "smoke_active",
  FLASH_PULSE: "flash_pulse",
});

const TYPE_ALIASES = {
  smoke: "smokegrenade",
  smokegrenade: "smokegrenade",
  flash: "flashbang",
  flashbang: "flashbang",
  he: "hegrenade",
  hegrenade: "hegrenade",
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

export const buildGrenadeKey = (grenade, channel = "main") => {
  const mIdx = grenade?.m_idx ?? "no_idx";
  return `${channel}:${mIdx}:${normalizeGrenadeType(grenade?.m_type)}`;
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
  const base = normalized === "molo" ? 320 : 240;
  const performancePenalty = settings?.grenadePerformanceMode ? -50 : 0;
  return clampNumber(Math.round(base + (intensity - 1) * 80 + performancePenalty), 160, 480);
};

export const getFlashPulseDurationMs = (settings) => {
  const intensity = getGrenadeEffectIntensity(settings);
  return clampNumber(Math.round(300 + (intensity - 1) * 140), 250, 450);
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
    zIndex: normalizedType === "molotov" || normalizedType === "incgrenade" ? 42 : 40,
  };
};

const KNOWN_GRENADE_ICONS = new Set([
  "smokegrenade",
  "flashbang",
  "hegrenade",
  "decoy",
  "molotov",
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
