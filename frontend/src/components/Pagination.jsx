export default function Pagination({ page, totalPages, onPageChange }) {
  const isFirst = page === 1;
  const isLast = page === totalPages;

  const pageNumbers = Array.from(
    { length: Math.min(5, totalPages) },
    (_, i) => Math.max(1, Math.min(page - 2, totalPages - 4)) + i,
  ).filter((p) => p > 0 && p <= totalPages);

  return (
    <div className="mt-5 flex justify-center gap-2">
      {/* Prev */}
      <button
        onClick={() => onPageChange(Math.max(1, page - 1))}
        disabled={isFirst}
        className={`rounded-lg border border-brand-pale bg-white px-5 py-2 text-[13px] font-medium transition-colors ${
          isFirst
            ? "cursor-not-allowed text-text-faint"
            : "cursor-pointer text-brand"
        }`}
      >
        ← Prev
      </button>

      {/* Page numbers */}
      {pageNumbers.map((p) => (
        <button
          key={p}
          onClick={() => onPageChange(p)}
          className={`cursor-pointer rounded-lg border-none px-4 py-2 text-[13px] transition-all ${
            page === p
              ? "bg-brand font-bold text-white shadow-active-page"
              : "bg-white font-normal text-text-secondary"
          }`}
        >
          {p}
        </button>
      ))}

      {/* Next */}
      <button
        onClick={() => onPageChange(Math.min(totalPages, page + 1))}
        disabled={isLast}
        className={`rounded-lg border border-brand-pale bg-white px-5 py-2 text-[13px] font-medium transition-colors ${
          isLast
            ? "cursor-not-allowed text-text-faint"
            : "cursor-pointer text-brand"
        }`}
      >
        Next →
      </button>
    </div>
  );
}
