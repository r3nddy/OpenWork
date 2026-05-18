import { useState } from "react";
import "./index.css";
import useJobs from "./hooks/useJobs.js";
import Header from "./components/Header.jsx";
import StatsBar from "./components/StatsBar.jsx";
import SearchBar from "./components/SearchBar.jsx";
import FilterBar from "./components/FilterBar.jsx";
import JobTable from "./components/JobTable.jsx";
import Pagination from "./components/Pagination.jsx";
import JobDetail from "./components/JobDetail.jsx";
import Toast from "./components/Toast.jsx";

export default function App() {
  const {
    jobsState,
    search,
    filter,
    page,
    setPage,
    username,
    bookmarked,
    toast,
    filtered,
    totalPages,
    paginated,
    handleSearch,
    handleFilterChange,
    handleBookmark,
    PAGE_SIZE,
  } = useJobs();

  const [selected, setSelected] = useState(null);

  return (
    <div className="min-h-screen bg-brand-bg">
      <Header username={username} />

      <main className="mx-auto max-w-300 px-6 py-7">
        {/* Stats */}
        {!jobsState.loading && !jobsState.error && (
          <StatsBar
            filtered={filtered}
            page={page}
            totalPages={totalPages}
            bookmarked={bookmarked}
          />
        )}

        {/* Toolbar */}
        <div className="mb-5 flex flex-wrap items-center gap-4">
          <SearchBar value={search} onChange={handleSearch} />
          <FilterBar active={filter} onChange={handleFilterChange} />
        </div>

        {/* Table card */}
        <div className="overflow-hidden rounded-[14px] border border-brand-border bg-white shadow-table">
          <JobTable
            loading={jobsState.loading}
            error={jobsState.error}
            paginated={paginated}
            page={page}
            pageSize={PAGE_SIZE}
            onDetail={setSelected}
            onBookmark={handleBookmark}
            bookmarked={bookmarked}
          />
        </div>

        {/* Pagination */}
        {!jobsState.loading && !jobsState.error && totalPages > 1 && (
          <Pagination
            page={page}
            totalPages={totalPages}
            onPageChange={setPage}
          />
        )}
      </main>

      {/* Detail panel */}
      <JobDetail
        job={selected}
        onClose={() => setSelected(null)}
        onBookmark={handleBookmark}
        bookmarked={selected && bookmarked.has(selected.id)}
      />

      {/* Toast */}
      <Toast message={toast} />
    </div>
  );
}
