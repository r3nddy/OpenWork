export default function Header({ username }) {
  return (
    <header className="sticky top-0 z-50 bg-linear-to-br from-brand-dark to-brand px-8 shadow-header">
      <div className="mx-auto flex h-16 max-w-300 items-center justify-between">
        {/* Logo */}
        <div className="flex items-center gap-3">
          <div className="flex h-9 w-9 items-center justify-center rounded-[10px] bg-white/20 text-xl">
            💼
          </div>
          <div>
            <span className="tracking-[-0.02em] text-lg font-extrabold text-white">
              Open<span className="text-brand-light">Work</span>
            </span>
            <div className="text-[11px] text-brand-pale">
              Hasil Scraping Lowongan
            </div>
          </div>
        </div>

        {/* User badge */}
        {username && (
          <div className="rounded-full bg-white/15 px-4 py-1.5 text-[13px] text-white">
            👤 {username}
          </div>
        )}
      </div>
    </header>
  );
}
