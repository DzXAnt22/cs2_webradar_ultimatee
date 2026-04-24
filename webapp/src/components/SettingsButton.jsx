import { useEffect, useMemo, useRef, useState } from "react";

const ToggleRow = ({ label, checked, onChange, indent = false }) => (
  <label
    className={`flex items-center justify-between rounded-xl px-3 py-2 transition-colors hover:bg-white/10 cursor-pointer ${
      indent ? "ml-5" : ""
    }`}
  >
    <span className="text-sm text-radar-secondary">{label}</span>
    <input
      type="checkbox"
      checked={checked}
      onChange={(event) => onChange(event.target.checked)}
      className="relative h-5 w-9 rounded-full shadow-sm bg-radar-secondary/30 checked:bg-radar-secondary transition-colors duration-200 appearance-none before:absolute before:h-4 before:w-4 before:top-0.5 before:left-0.5 before:bg-white before:rounded-full before:transition-transform before:duration-200 checked:before:translate-x-4"
    />
  </label>
);

const RangeRow = ({ label, valueLabel, min, max, step, value, onChange, indent = false }) => {
  const progress = useMemo(() => ((value - min) / (max - min)) * 100, [max, min, value]);

  return (
    <div className={`${indent ? "ml-5" : ""}`}>
      <div className="flex justify-between items-center mb-2 px-1">
        <span className="text-radar-secondary text-sm">{label}</span>
        <span className="text-radar-primary text-sm font-mono">{valueLabel}</span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(event) => onChange(parseFloat(event.target.value))}
        className="w-full h-2 rounded-lg appearance-none cursor-pointer accent-radar-primary"
        style={{
          background: `linear-gradient(to right, #b1d0e7 ${progress}%, rgba(59, 130, 246, 0.2) ${progress}%)`,
        }}
      />
    </div>
  );
};

const SettingsSection = ({ title, children }) => (
  <section className="glass-panel-soft rounded-xl p-3 space-y-2">
    <h4 className="text-xs uppercase tracking-[0.15em] text-radar-primary/80 font-semibold">{title}</h4>
    {children}
  </section>
);

const SettingsButton = ({ settings, onSettingsChange, translation, languageOptions }) => {
  const [isOpen, setIsOpen] = useState(false);
  const settingsMenu = useRef(null);
  const settingsButtonRef = useRef(null);

  const labels = {
    radar: translation.settings?.radar_section || "Radar",
    players: translation.settings?.players_section || "Players",
    grenades: translation.settings?.grenades_section || "Grenades",
    theme: translation.settings?.theme_section || "Theme",
    grenadeIntensity: translation.settings?.grenade_effect_intensity || "Effect Intensity",
    grenadeContrast: translation.settings?.grenade_high_contrast || "High Contrast Effects",
    flashPulse: translation.settings?.flash_pulse || "Flash Detonation Pulse",
    grenadeTimers: translation.settings?.show_grenade_timers || "Show Grenade Timers",
    grenadePerformance: translation.settings?.grenade_performance_mode || "Performance Mode",
  };

  useEffect(() => {
    const closeSettingsIfOpen = (event) => {
      if (
        isOpen &&
        !settingsMenu.current?.contains(event.target) &&
        !settingsButtonRef.current?.contains(event.target)
      ) {
        setIsOpen(false);
      }
    };

    document.addEventListener("mousedown", closeSettingsIfOpen);
    return () => {
      document.removeEventListener("mousedown", closeSettingsIfOpen);
    };
  }, [isOpen]);

  const updateSetting = (patch) => {
    onSettingsChange({ ...settings, ...patch });
  };

  return (
    <div className="z-50" ref={settingsButtonRef}>
      <button
        onClick={() => setIsOpen((previous) => !previous)}
        className="flex items-center gap-1.5 transition-all rounded-xl px-1 py-1 hover:bg-white/10"
      >
        <img className="w-[1.1rem]" src="./assets/icons/cog.svg" />
        <span className="text-radar-primary text-sm">{translation.settings?.button || "Settings"}</span>
      </button>

      {isOpen && (
        <div
          className="absolute right-0 mt-2 w-[22rem] max-w-[90vw] glass-panel rounded-2xl p-4 shadow-2xl"
          ref={settingsMenu}
        >
          <h3 className="text-radar-primary text-lg font-semibold mb-3">
            {translation.settings?.title || "Radar Settings"}
          </h3>

          <div className="space-y-3 overflow-y-auto max-h-[72vh] pr-1">
            <SettingsSection title={labels.radar}>
              <RangeRow
                label={translation.settings?.map_brightness || "Map Brightness"}
                valueLabel={`${settings.mapBrightness || 100}%`}
                min={10}
                max={195}
                step={5}
                value={settings.mapBrightness || 100}
                onChange={(value) => updateSetting({ mapBrightness: value })}
              />

              <RangeRow
                label={translation.settings?.bomb_size || "Bomb Size"}
                valueLabel={`${settings.bombSize}x`}
                min={0.1}
                max={2}
                step={0.1}
                value={settings.bombSize}
                onChange={(value) => updateSetting({ bombSize: value })}
              />

              <ToggleRow
                label={translation.settings?.follow_yourself || "Follow Yourself"}
                checked={settings.followYourself}
                onChange={(checked) => updateSetting({ followYourself: checked })}
              />

              {settings.followYourself && (
                <ToggleRow
                  indent
                  label={translation.settings?.follow_yourself_rotation || "Follow Rotation"}
                  checked={settings.followYourselfRotation}
                  onChange={(checked) => updateSetting({ followYourselfRotation: checked })}
                />
              )}

              <ToggleRow
                label={translation.settings?.view_player_cones || "View Player Cones"}
                checked={settings.showViewCones}
                onChange={(checked) => updateSetting({ showViewCones: checked })}
              />
            </SettingsSection>

            <SettingsSection title={labels.players}>
              <RangeRow
                label={translation.settings?.player_dot_size || "Player Dot Size"}
                valueLabel={`${settings.dotSize}x`}
                min={0.5}
                max={2}
                step={0.1}
                value={settings.dotSize}
                onChange={(value) => updateSetting({ dotSize: value })}
              />

              <ToggleRow
                label={translation.settings?.increase_player_contrast || "Increase Player Contrast"}
                checked={settings.increaseContrast}
                onChange={(checked) => updateSetting({ increaseContrast: checked })}
              />

              <ToggleRow
                label={translation.settings?.show_only_enemies || "Show Only Enemies"}
                checked={settings.showOnlyEnemies}
                onChange={(checked) => updateSetting({ showOnlyEnemies: checked })}
              />

              <ToggleRow
                label={translation.settings?.enemy_names || "Enemy Names"}
                checked={settings.showEnemyNames}
                onChange={(checked) => updateSetting({ showEnemyNames: checked })}
              />

              {!settings.showOnlyEnemies && (
                <ToggleRow
                  label={translation.settings?.ally_names || "Ally Names"}
                  checked={settings.showAllNames}
                  onChange={(checked) => updateSetting({ showAllNames: checked })}
                />
              )}
            </SettingsSection>

            <SettingsSection title={labels.grenades}>
              <ToggleRow
                label={translation.settings?.show_grenades || "Show Grenades"}
                checked={settings.showGrenades}
                onChange={(checked) => updateSetting({ showGrenades: checked })}
              />

              {settings.showGrenades && (
                <>
                  <label className="flex items-center justify-between rounded-xl px-3 py-2 hover:bg-white/10 cursor-pointer ml-5">
                    <span className="text-radar-secondary text-sm">
                      {translation.settings?.show_grenades_color || "Grenade Color"}
                    </span>
                    <input
                      type="color"
                      value={settings.thrownGrenadeColor}
                      onChange={(event) => updateSetting({ thrownGrenadeColor: event.target.value })}
                      className="relative h-5 w-9 rounded-full bg-radar-secondary/0"
                    />
                  </label>

                  <RangeRow
                    indent
                    label={translation.settings?.show_greandes_size || "Grenade Size"}
                    valueLabel={`${settings.thrownGrenadeSize}x`}
                    min={0.1}
                    max={2}
                    step={0.1}
                    value={settings.thrownGrenadeSize}
                    onChange={(value) => updateSetting({ thrownGrenadeSize: value })}
                  />

                  <RangeRow
                    indent
                    label={labels.grenadeIntensity}
                    valueLabel={`${settings.grenadeEffectIntensity.toFixed(1)}x`}
                    min={0.5}
                    max={1.8}
                    step={0.1}
                    value={settings.grenadeEffectIntensity}
                    onChange={(value) => updateSetting({ grenadeEffectIntensity: value })}
                  />

                  <ToggleRow
                    indent
                    label={labels.grenadeContrast}
                    checked={settings.grenadeHighContrast}
                    onChange={(checked) => updateSetting({ grenadeHighContrast: checked })}
                  />

                  <ToggleRow
                    indent
                    label={labels.flashPulse}
                    checked={settings.flashPulseEnabled}
                    onChange={(checked) => updateSetting({ flashPulseEnabled: checked })}
                  />

                  <ToggleRow
                    indent
                    label={labels.grenadeTimers}
                    checked={settings.showGrenadeTimers}
                    onChange={(checked) => updateSetting({ showGrenadeTimers: checked })}
                  />

                  <ToggleRow
                    indent
                    label={labels.grenadePerformance}
                    checked={settings.grenadePerformanceMode}
                    onChange={(checked) => updateSetting({ grenadePerformanceMode: checked })}
                  />
                </>
              )}
            </SettingsSection>

            <SettingsSection title={labels.theme}>
              <ToggleRow
                label={translation.settings?.show_dropped_weapons || "Show Dropped Weapons"}
                checked={settings.showDroppedWeapons}
                onChange={(checked) => updateSetting({ showDroppedWeapons: checked })}
              />

              {settings.showDroppedWeapons && (
                <>
                  <ToggleRow
                    indent
                    label={translation.settings?.show_dropped_weapons_lighter || "Use Lighter Color"}
                    checked={settings.droppedWeaponGlow}
                    onChange={(checked) => updateSetting({ droppedWeaponGlow: checked })}
                  />

                  <ToggleRow
                    indent
                    label={translation.settings?.show_dropped_weapons_ignore_grenades || "Ignore Grenades"}
                    checked={settings.droppedWeaponIgnoreNade}
                    onChange={(checked) => updateSetting({ droppedWeaponIgnoreNade: checked })}
                  />

                  <RangeRow
                    indent
                    label={translation.settings?.show_dropped_weapons_size || "Weapon Size"}
                    valueLabel={`${settings.droppedWeaponSize}x`}
                    min={0.1}
                    max={2}
                    step={0.1}
                    value={settings.droppedWeaponSize}
                    onChange={(value) => updateSetting({ droppedWeaponSize: value })}
                  />
                </>
              )}

              <label className="flex items-center justify-between rounded-xl px-3 py-2 hover:bg-white/10 cursor-pointer">
                <span className="text-radar-secondary text-sm">
                  {translation.settings?.theme_color_text || "Theme Color"}
                </span>
                <select
                  value={settings.colorScheme}
                  onChange={(event) => updateSetting({ colorScheme: event.target.value })}
                  className="ml-2 bg-radar-panel text-radar-primary rounded-md px-2 py-1 text-sm focus:outline-none"
                  style={{ background: "rgba(59, 130, 246, 0.2)", border: "none" }}
                >
                  <option value="default">{translation.settings?.theme_colors?.default || "Default"}</option>
                  <option value="white">{translation.settings?.theme_colors?.white || "White"}</option>
                  <option value="light_blue">{translation.settings?.theme_colors?.light_blue || "Light Blue"}</option>
                  <option value="dark_blue">{translation.settings?.theme_colors?.dark_blue || "Dark Blue"}</option>
                  <option value="purple">{translation.settings?.theme_colors?.purple || "Purple"}</option>
                  <option value="red">{translation.settings?.theme_colors?.red || "Red"}</option>
                  <option value="orange">{translation.settings?.theme_colors?.orange || "Orange"}</option>
                  <option value="yellow">{translation.settings?.theme_colors?.yellow || "Yellow"}</option>
                  <option value="green">{translation.settings?.theme_colors?.green || "Green"}</option>
                  <option value="light_green">{translation.settings?.theme_colors?.light_green || "Light Green"}</option>
                  <option value="pink">{translation.settings?.theme_colors?.pink || "Pink"}</option>
                </select>
              </label>

              <label className="flex items-center justify-between rounded-xl px-3 py-2 hover:bg-white/10 cursor-pointer">
                <span className="text-radar-secondary text-sm">{translation.settings?.language || "Language"}</span>
                <select
                  value={settings.language}
                  onChange={(event) => updateSetting({ language: event.target.value })}
                  className="ml-2 bg-radar-panel text-radar-primary rounded-md px-2 py-1 text-sm focus:outline-none"
                  style={{ background: "rgba(59, 130, 246, 0.2)", border: "none" }}
                >
                  {languageOptions.map((langName) => (
                    <option key={langName} value={langName}>
                      {langName}
                    </option>
                  ))}
                </select>
              </label>

              <button
                className="flex items-center justify-center p-3 bg-radar-redbutton rounded-lg hover:bg-radar-redbutton_hover transition-colors cursor-pointer w-full"
                onClick={() => updateSetting({ whichPlayerAreYou: "0" })}
              >
                <span className="text-white text-sm">
                  {translation.settings?.choose_yourself_again_button || "Choose Yourself Again"}
                </span>
              </button>
            </SettingsSection>
          </div>
        </div>
      )}
    </div>
  );
};

export default SettingsButton;
