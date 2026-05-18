const SOURCES = ["semua", "jobstreet", "kitalulus", "lokerid", "bookmark"];

export default function FilterBar({ active, onChange }) {
  return (
    <div className="flex flex-wrap gap-2">
      {SOURCES.map((src) => {
        const isActive = active === src;
        const getLabel = (s) => {
          if (s === "semua") return "🔍 Semua";
          if (s === "bookmark") return "🔖 Tersimpan";
          return s;
        };
        
        return (
          <button
            key={src}
            onClick={() => onChange(src)}
            className={`cursor-pointer rounded-full px-5 py-1.5 text-[13px] capitalize transition-all duration-150 ${
              isActive
                ? "border-none bg-brand font-semibold text-white"
                : "border border-brand-pale bg-white font-normal text-brand"
            }`}
          >
            {getLabel(src)}
          </button>
        );
      })}
    </div>
  );
}
