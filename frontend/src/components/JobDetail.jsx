function formatSalary(min, max) {
  if (!min && !max) return "Tidak dicantumkan";
  if (min && max) return `${min} – ${max} juta`;
  if (max) return `Sampai ${max} juta`;
  return `${min}+ juta`;
}

const META_FIELDS = (job, salary) => [
  ["📍 Lokasi", job.location || "—"],
  ["💰 Gaji", salary],
  ["🏷 Sumber", job.source || "—"],
  ["📅 Scraped", job.scraped_at ? job.scraped_at.split("T")[0] : "—"],
];

export default function JobDetail({ job, onClose, onBookmark, bookmarked }) {
  if (!job) return null;

  const salary = typeof job.salary === 'string' && job.salary !== "N/A" 
                 ? job.salary + (job.salary.includes("juta") ? "" : " juta")
                 : formatSalary(job.salary_min, job.salary_max);

  return (
    <>
      {/* Backdrop */}
      <div
        onClick={onClose}
        className="fixed inset-0 z-100 bg-slate-900/45 backdrop-blur-[2px] transition-opacity duration-200"
      />

      {/* Panel */}
      <div className="fixed top-0 right-0 bottom-0 z-101 flex w-[min(520px,100vw)] flex-col overflow-y-auto bg-white shadow-panel animate-slide-in">
        {/* Header */}
        <div className="bg-linear-to-br from-brand-dark to-brand px-7 pt-6 pb-5 text-white">
          <button
            onClick={onClose}
            className="mb-4 cursor-pointer rounded-lg border-none bg-white/15 px-3 py-1 text-[13px] text-white"
          >
            ← Kembali
          </button>
          <h2 className="mb-1.5 text-xl font-bold leading-tight">
            {job.title || "—"}
          </h2>
          <div className="text-[15px] opacity-88">{job.company || "—"}</div>
        </div>

        {/* Body */}
        <div className="flex-1 px-7 py-6">
          {/* Meta grid */}
          <div className="mb-6 grid grid-cols-2 gap-4">
            {META_FIELDS(job, salary).map(([label, val]) => (
              <div
                key={label}
                className="rounded-[10px] border border-brand-border bg-brand-bg p-4"
              >
                <div className="mb-1 text-[11px] text-text-muted">{label}</div>
                <div className="text-sm font-semibold text-text-primary">
                  {val}
                </div>
              </div>
            ))}
          </div>

          {/* Description */}
          {job.description && (
            <div className="mb-6">
              <h3 className="mb-2.5 text-[14px] font-bold uppercase tracking-wide text-brand-dark">
                Deskripsi
              </h3>
              <p className="whitespace-pre-wrap text-[14px] leading-relaxed text-text-dark">
                {job.description}
              </p>
            </div>
          )}

          {/* URL */}
          {job.url && job.url !== "N/A" && (
            <a
              href={job.url}
              target="_blank"
              rel="noreferrer"
              className="mb-6 block break-all rounded-lg border border-brand-pale bg-brand-row px-4 py-3 text-[13px] text-brand no-underline transition-colors"
            >
              🔗 Lihat lowongan asli
            </a>
          )}

          {/* Bookmark button */}
          <button
            onClick={() => onBookmark(job)}
            className={`w-full cursor-pointer rounded-[10px] border-none p-3 text-[15px] font-semibold transition-all duration-150 ${
              bookmarked
                ? "bg-bookmark-bg text-bookmark-text shadow-none"
                : "bg-brand text-white shadow-bookmark"
            }`}
          >
            {bookmarked ? "✓ Sudah Tersimpan" : "+ Simpan ke Bookmark"}
          </button>
        </div>
      </div>
    </>
  );
}
