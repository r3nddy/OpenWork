const BADGE_CLASSES = {
  jobstreet: "bg-badge-jobstreet-bg text-badge-jobstreet-text",
  kitalulus: "bg-badge-kitalulus-bg text-badge-kitalulus-text",
  lokerid:   "bg-badge-lokerid-bg text-badge-lokerid-text",
};
const BADGE_DEFAULT = "bg-badge-default-bg text-badge-default-text";

function formatSalary(min, max) {
  if (!min && !max) return "—";
  if (min && max) return `${min} – ${max} jt`;
  if (max) return `s.d. ${max} jt`;
  return `${min}+ jt`;
}

export default function JobRow({ job, index, onDetail, onBookmark, bookmarked }) {
  const src = (job.source || "unknown").toLowerCase();
  const badgeClass = BADGE_CLASSES[src] || BADGE_DEFAULT;
  let salary = "—";
  if (typeof job.salary === "string" && job.salary !== "N/A" && job.salary.trim() !== "") {
    const numValue = Number(job.salary);
    if (!isNaN(numValue)) {
      salary = new Intl.NumberFormat("id-ID", {
        style: "currency",
        currency: "IDR",
        maximumFractionDigits: 0
      }).format(numValue);
    } else {
      salary = job.salary;
    }
  } else {
    salary = formatSalary(job.salary_min, job.salary_max);
  }

  return (
    <tr className="border-b border-brand-border bg-white transition-colors duration-150 hover:bg-brand-row">
      <td className="whitespace-nowrap px-4 py-3 text-[13px] text-text-dim">
        {index + 1}
      </td>
      <td className="px-4 py-3">
        <div className="mb-0.5 text-sm font-semibold text-text-primary">
          {job.title || "—"}
        </div>
        <div className="text-[12px] text-text-muted">{job.company || "—"}</div>
      </td>
      <td className="px-4 py-3 text-[13px] text-text-secondary">
        {job.location || "—"}
      </td>
      <td className="px-4 py-3 text-[13px] font-medium text-brand">
        {salary}
      </td>
      <td className="px-4 py-3">
        <span
          className={`inline-block rounded-full px-2.5 py-0.5 text-[11px] font-semibold uppercase tracking-wide ${badgeClass}`}
        >
          {src}
        </span>
      </td>
      <td className="px-4 py-3">
        <div className="flex gap-2">
          <button
            onClick={() => onDetail(job)}
            className="cursor-pointer rounded-md border border-brand bg-white px-3 py-1 text-[12px] font-medium text-brand transition-all duration-150 hover:bg-brand hover:text-white"
          >
            Detail
          </button>
          <button
            onClick={() => onBookmark(job)}
            className={`cursor-pointer rounded-md border-none px-3 py-1 text-[12px] font-medium transition-all duration-150 ${
              bookmarked
                ? "bg-bookmark-bg text-bookmark-text"
                : "bg-brand text-white"
            }`}
          >
            {bookmarked ? "✓ Tersimpan" : "+ Simpan"}
          </button>
        </div>
      </td>
    </tr>
  );
}
