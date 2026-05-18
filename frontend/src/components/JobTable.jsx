import JobRow from "./JobRow.jsx";

const TABLE_HEADERS = [
  "No",
  "Posisi & Perusahaan",
  "Lokasi",
  "Gaji",
  "Sumber",
  "Aksi",
];

export default function JobTable({
  loading,
  error,
  paginated,
  page,
  pageSize,
  onDetail,
  onBookmark,
  bookmarked,
}) {
  if (loading) {
    return (
      <div className="py-20 text-center text-brand">
        <div className="mb-3 text-[40px]">⏳</div>
        <div className="font-semibold">Memuat data...</div>
        <div className="mt-1 text-[13px] text-text-dim">
          Menghubungi API server...
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="py-20 text-center text-error">
        <div className="mb-3 text-[40px]">⚠️</div>
        <div className="font-semibold">Gagal memuat data</div>
        <div className="mt-1 text-[13px] text-text-dim">
          Pastikan C++ app dan API server sudah berjalan.
        </div>
        <button
          onClick={() => window.location.reload()}
          className="mt-4 cursor-pointer rounded-lg border-none bg-brand px-5 py-2 text-[13px] text-white"
        >
          Coba Lagi
        </button>
      </div>
    );
  }

  if (paginated.length === 0) {
    return (
      <div className="py-20 text-center text-text-dim">
        <div className="mb-3 text-[40px]">🔍</div>
        <div className="font-semibold text-text-secondary">
          Tidak ada lowongan ditemukan
        </div>
      </div>
    );
  }

  return (
    <div className="overflow-x-auto">
      <table className="w-full min-w-175 border-collapse">
        <thead>
          <tr className="border-b-2 border-brand-border bg-brand-row">
            {TABLE_HEADERS.map((h) => (
              <th
                key={h}
                className="whitespace-nowrap px-4 py-3 text-left text-[12px] font-bold uppercase tracking-wide text-brand"
              >
                {h}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {paginated.map((job, i) => (
            <JobRow
              key={job.id || i}
              job={job}
              index={(page - 1) * pageSize + i}
              onDetail={onDetail}
              onBookmark={onBookmark}
              bookmarked={bookmarked.has(job.id)}
            />
          ))}
        </tbody>
      </table>
    </div>
  );
}
