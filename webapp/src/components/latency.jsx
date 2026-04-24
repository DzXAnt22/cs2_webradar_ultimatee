import SettingsButton from "./SettingsButton";

let latencyData = {
  averageCount: 0,
  averageSum: 0,
  averageTime: 0,
  lastTime: new Date().getTime(),
};

export const getLatency = () => {
  const currentTime = new Date().getTime();
  const diffInMs = currentTime - latencyData.lastTime;
  latencyData.lastTime = currentTime;

  if (latencyData.averageTime === 0) {
    latencyData.averageTime = diffInMs;
  }

  latencyData.averageCount += 1;
  latencyData.averageSum += diffInMs;

  if (latencyData.averageCount >= 5) {
    latencyData.averageTime = latencyData.averageSum / latencyData.averageCount;
    latencyData.averageCount = 0;
    latencyData.averageSum = 0;
  }

  return latencyData.averageTime;
};

const getLatencyTone = (value) => {
  if (value >= 120) {
    return "text-red-300";
  }

  if (value >= 75) {
    return "text-amber-200";
  }

  return "text-emerald-300";
};

export const Latency = ({ value, settings, setSettings, translation, languages }) => {
  return (
    <div className="absolute right-4 top-4 z-50">
      <div className="glass-panel rounded-2xl px-3 py-2 flex items-center gap-3">
        <div className="flex items-center gap-1.5 text-sm">
          <img className="w-[1.1rem] opacity-90" src="./assets/icons/gauge.svg" />
          <span className={`${getLatencyTone(value)} font-semibold`}>{value.toFixed(0)}ms</span>
        </div>

        <div className="h-5 w-px bg-white/15" />

        <SettingsButton
          settings={settings}
          onSettingsChange={setSettings}
          translation={translation}
          languageOptions={languages}
        />
      </div>
    </div>
  );
};
