import { useState, useEffect, useMemo, useCallback } from "react";
import axios from "axios";

const PAGE_SIZE = 20;

/**
 * Custom hook — manages job fetching, filtering, search, pagination, and bookmarks.
 */
export default function useJobs() {
  const [jobsState, setJobsState] = useState({
    items: [],
    loading: true,
    error: null,
  });

  const [search, setSearch] = useState("");
  const [filter, setFilter] = useState("semua");
  const [page, setPage] = useState(1);
  const [username, setUsername] = useState("");
  const [bookmarked, setBookmarked] = useState(new Set());
  const [toast, setToast] = useState("");

  /* ── Fetch session & initial bookmarks ── */
  useEffect(() => {
    axios
      .get("/api/session")
      .then((response) => {
        setUsername(response.data.username || "");
        return axios.get("/api/bookmarks");
      })
      .then((response) => {
        if (response && response.data) {
          const bms = Array.isArray(response.data) ? response.data : [];
          const bmIds = new Set(bms.map((b) => b.sourceId || b.id));
          setBookmarked(bmIds);
        }
      })
      .catch(() => {});
  }, []);

  /* ── Fetch jobs ── */
  useEffect(() => {
    const controller = new AbortController();
    
    let url = "";
    if (filter === "bookmark") {
      url = "/api/bookmarks";
    } else if (filter === "semua") {
      url = "/api/scrape";
    } else {
      url = `/api/scrape?source=${filter}`;
    }

    axios
      .get(url, { signal: controller.signal })
      .then((response) => {
        let items = Array.isArray(response.data) ? response.data : [];
        
        // Normalize bookmarked items to match scraped items shape
        if (filter === "bookmark") {
            items = items.map(item => ({
                ...item,
                id: item.sourceId || item.id,
                salary_min: item.salary ? undefined : item.salary_min,
                salary_max: item.salary ? undefined : item.salary_max
            }));
        }

        setJobsState({
          items: items,
          loading: false,
          error: null,
        });
        setPage(1);
      })
      .catch((error) => {
        if (axios.isCancel(error)) return;
        setJobsState((prev) => ({
          ...prev,
          loading: false,
          error: error.message,
        }));
      });

    return () => controller.abort();
  }, [filter]);

  /* ── Derived: filtered + paginated ── */
  const filtered = useMemo(() => {
    if (!search.trim()) return jobsState.items;
    const q = search.toLowerCase();
    return jobsState.items.filter(
      (j) =>
        (j.title || "").toLowerCase().includes(q) ||
        (j.company || "").toLowerCase().includes(q),
    );
  }, [jobsState.items, search]);

  const totalPages = Math.max(1, Math.ceil(filtered.length / PAGE_SIZE));
  const paginated = filtered.slice((page - 1) * PAGE_SIZE, page * PAGE_SIZE);

  /* ── Handlers ── */
  const handleSearch = useCallback((val) => {
    setSearch(val);
    setPage(1);
  }, []);

  const handleFilterChange = useCallback((f) => {
    setJobsState((prev) => ({ ...prev, loading: true, error: null }));
    setFilter(f);
    setSearch("");
  }, []);

  const showToast = useCallback((msg) => {
    setToast(msg);
    setTimeout(() => setToast(""), 3000);
  }, []);

  const handleBookmark = useCallback(
    async (job) => {
      if (!job.id) return;
      const isAlreadyBookmarked = bookmarked.has(job.id);

      try {
        if (isAlreadyBookmarked) {
          // REMOVE BOOKMARK
          const response = await axios.delete("/api/bookmark", {
            data: { username, jobId: job.id },
          });

          if (response.data.status === "success") {
            setBookmarked((prev) => {
              const next = new Set(prev);
              next.delete(job.id);
              return next;
            });
            
            // If on bookmark filter, remove from list immediately
            if (filter === "bookmark") {
              setJobsState(prev => ({
                ...prev,
                items: prev.items.filter(item => (item.id || item.sourceId) !== job.id)
              }));
            }
            
            showToast(`✓ Bookmark "${job.title}" dihapus.`);
          } else {
            showToast("Gagal menghapus bookmark.");
          }
        } else {
          // ADD BOOKMARK
          const response = await axios.post("/api/bookmark", {
            username,
            jobId: job.id,
          });

          if (response.data.status === "success") {
            setBookmarked((prev) => new Set([...prev, job.id]));
            showToast(`✓ "${job.title}" disimpan ke bookmark!`);
          } else {
            showToast("Gagal menyimpan bookmark.");
          }
        }
      } catch (err) {
        console.error("Bookmark error:", err);
        showToast("Server tidak merespons.");
      }
    },
    [username, bookmarked, filter, showToast],
  );

  return {
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
  };
}
