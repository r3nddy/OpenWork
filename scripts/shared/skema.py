"""
schemas.py — Config constants, shared paths & data schemas
Semua path penting dipusatkan di sini agar konsisten
antar modul scraper dan agent.
"""

from pathlib import Path
from typing import Optional

# Root project (parent dari services/)
PROJECT_ROOT = Path(__file__).parent.parent.parent.resolve()

# Folder services (parent dari shared/)
SERVICES_DIR = Path(__file__).parent.parent.resolve()

# Folder data — semua output JSON disimpan di sini
DATA_DIR = PROJECT_ROOT / "data"

# ── Scrape paths ──
SCRAPE_DIR = DATA_DIR / "scrape"
HASIL_JOBSTREET_FILE = SCRAPE_DIR / "hasil_jobstreet.json"
HASIL_KITALULUS_FILE = SCRAPE_DIR / "hasil_kitalulus.json"
HASIL_LOKERID_FILE = SCRAPE_DIR / "hasil_lokerid.json"
MERGED_DATA_FILE = SCRAPE_DIR / "merged_data.json"
STATE_FILE = DATA_DIR / "state.json"

# Backward compat aliases
SCRAPING_DIR = SCRAPE_DIR
HASIL_GABUNGAN_FILE = MERGED_DATA_FILE

# ── Data paths (sama dengan yang dipakai C++) ──
PROFILE_FILE = DATA_DIR / "profile.json"
LOWONGAN_FILE = DATA_DIR / "lowongan.json"
BOOKMARKED_JOBS_FILE = DATA_DIR / "bookmarked_jobs.json"

# ── Scraper config ──
BASE_URL_JOBSTREET = "https://id.jobstreet.com"
DEFAULT_MAX_PAGE = 5


#  Data Schemas — untuk komunikasi Matcher & Analyst

# ── Dokumentasi Format Dictionary ──
#
# MatchingResult dict:
#   - job_data: dict          # Data lowongan lengkap
#   - user_id: str            # Username user
#   - score: int              # Skor kecocokan 0-100
#   - auto_bookmarked: bool   # Apakah sudah di-bookmark otomatis
#
# BookmarkRequest dict:
#   - user_id: str            # Username user
#   - job_id: str             # ID lowongan (sourceId)
#   - source: str             # "auto" atau "manual"
#   - score: int              # Skor kecocokan (0 jika manual)
