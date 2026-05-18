export default function StatsBar({ filtered, page, totalPages, bookmarked }) {
  const stats = [
    ["Total Lowongan", filtered.length],
    ["Halaman", `${page} / ${totalPages}`],
    ["Tersimpan", bookmarked.size],
  ];

  return (
    <div className="mb-5 flex flex-wrap gap-4">
      {stats.map(([label, val]) => (
        <div
          key={label}
          className="min-w-30 rounded-[10px] border border-brand-border bg-white px-5 py-3 text-center shadow-card"
        >
          <div className="text-[22px] font-bold text-brand">{val}</div>
          <div className="mt-0.5 text-[11px] text-text-dim">{label}</div>
        </div>
      ))}
    </div>
  );
}
