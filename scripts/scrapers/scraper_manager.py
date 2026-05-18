"""
Orkestrator semua scraper secara paralel, menangani penulisan file
secara atomic, melakukan deduplikasi lintas sumber, dan memperbarui state.
"""

import asyncio
import json
import os
from datetime import datetime, timezone

from shared.skema import (
    HASIL_JOBSTREET_FILE,
    HASIL_KITALULUS_FILE,
    HASIL_LOKERID_FILE,
    HASIL_GABUNGAN_FILE,
    STATE_FILE,
    SCRAPING_DIR,
    DEFAULT_MAX_PAGE,
)
from scrapers.utils import deduplicate_cross_source

# Import scraper
from scrapers import jobstreet_scraper
from scrapers import kitalulus_scraper
from scrapers import lokerid_scraper

SCRAPERS = [
    (jobstreet_scraper.get_source_name(), jobstreet_scraper.scrape, HASIL_JOBSTREET_FILE),
    (kitalulus_scraper.get_source_name(), kitalulus_scraper.scrape, HASIL_KITALULUS_FILE),
    (lokerid_scraper.get_source_name(), lokerid_scraper.scrape, HASIL_LOKERID_FILE),
]

def update_state(state_data: dict, status: str = None, total_gabungan: int = 0, total_duplikat: int = 0):
    """Membaca state lama, update, lalu simpan dengan atomic write."""
    current = {}
    if STATE_FILE.exists():
        try:
            with open(STATE_FILE, "r", encoding="utf-8") as f:
                current = json.load(f)
        except Exception:
            pass
            
    scraper_state = current.get("scraper", {
        "state": "idle",
        "last_run": "",
        "sources": {
            "jobstreet": {"state": "skipped", "total": 0, "error": None},
            "kitalulus": {"state": "skipped", "total": 0, "error": None},
            "lokerid": {"state": "skipped", "total": 0, "error": None},
        },
        "total_gabungan": 0,
        "total_duplikat_dihapus": 0
    })
    
    if status:
        scraper_state["state"] = status
        if status in ["done", "error"]:
            scraper_state["last_run"] = datetime.now(timezone.utc).isoformat()
            
    # Update sources jika ada info baru
    for src, src_data in state_data.items():
        if src in scraper_state["sources"]:
            scraper_state["sources"][src].update(src_data)
            
    scraper_state["total_gabungan"] = total_gabungan
    scraper_state["total_duplikat_dihapus"] = total_duplikat
    
    current["scraper"] = scraper_state
    
    # Save (tanpa atomic karena ini file state kecil, tp idealnya atomic)
    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = str(STATE_FILE) + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(current, f, indent=2, ensure_ascii=False)
    os.replace(tmp_path, str(STATE_FILE))

def atomic_write_json(file_path, data):
    """Tulis JSON ke file .tmp lalu replace file asli untuk menghindari korupsi data."""
    path = str(file_path)
    tmp_path = path + ".tmp"
    
    # Pastikan folder target ada
    file_path.parent.mkdir(parents=True, exist_ok=True)
    
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        
    os.replace(tmp_path, path)


async def run_scraper_safe(source_name, scrape_func, file_path, keyword, location, max_page):
    """Menjalankan satu scraper dan menangkap exception jika gagal."""
    print(f"[{source_name.upper()}] Memulai scraping...")
    try:
        jobs = await scrape_func(keyword, location, max_page)
        if jobs:
            # Deduplikasi berdasarkan ID untuk mencegah item bersponsor muncul berulang kali
            seen_ids = set()
            unique_jobs = []
            for j in jobs:
                jid = j.get("id")
                if not jid:
                    unique_jobs.append(j)
                elif jid not in seen_ids:
                    seen_ids.add(jid)
                    unique_jobs.append(j)
            
            atomic_write_json(file_path, unique_jobs)
            print(f"[{source_name.upper()}] Selesai. Mendapatkan {len(unique_jobs)} lowongan unik (dari {len(jobs)} mentah).")
            return source_name, unique_jobs, None
        else:
            print(f"[{source_name.upper()}] Selesai. Tidak ada lowongan.")
            return source_name, [], None
    except Exception as e:
        print(f"[{source_name.upper()}] ERROR: {e}")
        return source_name, [], str(e)


async def run_all(keyword: str, location: str, max_page: int):
    """Menjalankan semua scraper secara paralel."""
    print("=" * 60)
    print("  MEMULAI SCRAPING PARALEL")
    print("=" * 60)
    
    # Setup state awal -> running
    update_state({}, status="running")
    
    tasks = []
    for source_name, scrape_func, file_path in SCRAPERS:
        tasks.append(
            run_scraper_safe(source_name, scrape_func, file_path, keyword, location, max_page)
        )
        
    # Jalankan semua secara bersamaan
    results = await asyncio.gather(*tasks, return_exceptions=True)
    
    all_jobs_gabungan = []
    state_updates = {}
    
    # Kumpulkan hasil
    for res in results:
        if isinstance(res, Exception):
            # Fallback kalau error di luar tangkapan
            print(f"[MANAGER] Unexpected error: {res}")
            continue
            
        source_name, jobs, error_msg = res
        
        if error_msg:
            state_updates[source_name] = {"state": "error", "total": 0, "error": error_msg}
        else:
            state_updates[source_name] = {"state": "done", "total": len(jobs), "error": None}
            all_jobs_gabungan.extend(jobs)
            
    # Deduplikasi
    print(f"\n[MANAGER] Melakukan deduplikasi dari total {len(all_jobs_gabungan)} lowongan...")
    unique_jobs, dup_count = deduplicate_cross_source(all_jobs_gabungan)
    
    # Simpan hasil gabungan
    if unique_jobs:
        atomic_write_json(HASIL_GABUNGAN_FILE, unique_jobs)
        print(f"[MANAGER] Tersimpan {len(unique_jobs)} lowongan unik ke {HASIL_GABUNGAN_FILE}")
        print(f"[MANAGER] Menghapus {dup_count} lowongan duplikat.")
    else:
        print("[MANAGER] Tidak ada lowongan yang berhasil dikumpulkan.")
        
    # Update state akhir -> done
    update_state(state_updates, status="done", total_gabungan=len(unique_jobs), total_duplikat=dup_count)
    print("=" * 60)
    print("  SCRAPING SELESAI")
    print("=" * 60)

def ask_user_input():
    """Minta input dari user di terminal."""
    print("=" * 60)
    print("  MULTI-SOURCE SCRAPER")
    print("=" * 60)
    keyword = input("\n  Mau cari lowongan apa? (contoh: python, data analyst): ").strip()
    if not keyword:
        keyword = "python"
        print(f"  (default: '{keyword}')")
    location = input("  Lokasi? (kosongkan jika semua lokasi): ").strip()
    try:
        max_page = input(f"  Berapa halaman per sumber? (default: {DEFAULT_MAX_PAGE}): ").strip()
        max_page = int(max_page) if max_page else DEFAULT_MAX_PAGE
    except ValueError:
        max_page = DEFAULT_MAX_PAGE
    print()
    return keyword, location, max_page

def run_manager():
    """Entry point untuk manager (dipanggil dari main.py)."""
    keyword, location, max_page = ask_user_input()
    asyncio.run(run_all(keyword, location, max_page))

if __name__ == "__main__":
    run_manager()
