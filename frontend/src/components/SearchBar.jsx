export default function SearchBar({ value, onChange }) {
  return (
    <div className="relative min-w-55 flex-1">
      <span className="pointer-events-none absolute left-3 top-1/2 -translate-y-1/2 text-base text-text-dim">
        🔎
      </span>
      <input
        type="text"
        placeholder="Cari judul atau perusahaan..."
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className="w-full rounded-lg border border-brand-pale bg-white px-3 py-2.5 pl-9 text-sm text-text-primary shadow-search outline-none transition-all duration-150 focus:border-brand"
      />
    </div>
  );
}
