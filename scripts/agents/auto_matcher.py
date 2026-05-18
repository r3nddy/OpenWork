"""
auto_matcher.py — Automated Job Matching & Auto-Bookmarking
Setelah scraping selesai, modul ini:
  1. Mengambil profil user dari C++ via bridge
  2. Membaca lowongan hasil scrape dari merged_data.json
  3. Mencocokkan setiap lowongan dengan profil (scoring 0-100) via LLM
  4. Lowongan yang memenuhi threshold langsung di-bookmark via bridge
  5. Lowongan di bawah threshold diabaikan (tanpa error)
"""

import json
from pathlib import Path

from shared.skema import (
    MERGED_DATA_FILE,
)
from shared.bridge import get_user_profile, send_bookmark_request
from agents.client import buat_client_openrouter, chat_qwen

# Config
MATCH_THRESHOLD = 70  # Skor minimal untuk auto-bookmark

SYSTEM_PROMPT_SCORER = (
    "Kamu adalah sistem penilaian kecocokan lowongan kerja. "
    "Kamu akan menerima profil pencari kerja dan satu lowongan kerja. "
    "Tugasmu HANYA mengembalikan skor kecocokan berupa angka bulat 0-100. "
    "Pertimbangkan kecocokan skills, lokasi, gaji, dan bidang. "
    "JAWAB HANYA DENGAN ANGKA. Contoh: 75"
)


def _parse_score(response: str) -> int:
    """Ekstrak skor dari respons LLM. Cari angka pertama 0-100."""
    # Coba langsung parse
    text = response.strip()
    try:
        val = int(text)
        return max(0, min(100, val))
    except ValueError:
        pass

    # Cari angka dalam teks
    import re
    numbers = re.findall(r'\d+', text)
    for n in numbers:
        val = int(n)
        if 0 <= val <= 100:
            return val
    return 0


def _build_profile_text(profile: dict) -> str:
    """Bangun teks ringkasan profil untuk dikirim ke LLM."""
    skills_list = profile.get('skills', [])
    skills_formatted = []
    if isinstance(skills_list, list):
        for s in skills_list:
            if isinstance(s, dict):
                name = s.get('name', 'N/A')
                level = s.get('level', 'N/A')
                skills_formatted.append(f"{name} (level {level})")
            else:
                skills_formatted.append(str(s))
    skills_str = ", ".join(skills_formatted) if skills_formatted else "N/A"

    return (
        f"Nama: {profile.get('name', 'N/A')}\n"
        f"Lokasi: {profile.get('location', 'N/A')}\n"
        f"Ekspektasi Gaji: {profile.get('salaryExpectation', 'N/A')}\n"
        f"Skills: {skills_str}"
    )


def _build_job_text(job: dict) -> str:
    """Bangun teks ringkasan lowongan untuk scoring."""
    title = job.get('title', 'N/A')
    company = job.get('company', 'N/A')
    location = job.get('location', 'N/A')

    salary_min = job.get('salary_min')
    salary_max = job.get('salary_max')
    if salary_min or salary_max:
        salary = f"Rp {salary_min or '?'} - {salary_max or '?'}"
    else:
        salary = job.get('salary', 'N/A')

    desc = job.get('description') or job.get('teaser') or 'N/A'
    field = job.get('field', '')

    text = f"Title: {title}\nCompany: {company}\nLokasi: {location}\nGaji: {salary}\nDeskripsi: {desc}"
    if field:
        text += f"\nBidang: {field}"
    return text


def run_auto_match(username: str) -> None:
    """
    Alur utama auto-match:
    1. Ambil profil user dari C++ via bridge
    2. Baca lowongan dari merged_data.json
    3. Scoring via LLM
    4. Auto-bookmark jika skor >= threshold
    """
    print("=" * 60)
    print("  AUTO-MATCH: Pencocokan Otomatis Lowongan")
    print("=" * 60)

    # 1. Ambil profil user
    print(f"\n[1/4] Mengambil profil user '{username}' dari C++...")
    profile = get_user_profile(username)
    if not profile or not profile.get('name'):
        print(f"[SKIP] Profil user '{username}' tidak ditemukan atau kosong.")
        print("       Auto-match dibatalkan. Lowongan tetap tersimpan.")
        return

    print(f"  -> Profil: {profile.get('name')} ({profile.get('location', 'N/A')})")

    # 2. Baca lowongan dari merged_data
    print(f"\n[2/4] Membaca lowongan dari {MERGED_DATA_FILE.name}...")
    if not MERGED_DATA_FILE.exists():
        print("[SKIP] File merged_data.json tidak ditemukan.")
        return

    with open(MERGED_DATA_FILE, "r", encoding="utf-8") as f:
        jobs = json.load(f)

    if not isinstance(jobs, list) or len(jobs) == 0:
        print("[SKIP] Tidak ada lowongan untuk dicocokkan.")
        return

    print(f"  -> Ditemukan {len(jobs)} lowongan")

    # 3. Scoring via LLM
    print(f"\n[3/4] Mencocokkan {len(jobs)} lowongan dengan profil (LLM scoring)...")
    profile_text = _build_profile_text(profile)

    try:
        client = buat_client_openrouter()
    except Exception as e:
        print(f"[SKIP] Gagal membuat koneksi LLM: {e}")
        print("       Auto-match dibatalkan. Lowongan tetap tersimpan.")
        return

    results: list[dict] = []
    bookmarked_count = 0

    for i, job in enumerate(jobs):
        job_id = ""
        if "id" in job and job["id"] is not None:
            job_id = str(job["id"])
        
        if not job_id:
            continue

        job_text = _build_job_text(job)
        title = job.get('title', 'N/A')

        # Kirim ke LLM untuk scoring
        messages = [
            {"role": "system", "content": SYSTEM_PROMPT_SCORER},
            {
                "role": "user",
                "content": (
                    f"PROFIL PENCARI KERJA:\n{profile_text}\n\n"
                    f"LOWONGAN KERJA:\n{job_text}\n\n"
                    f"Berikan skor kecocokan (0-100):"
                ),
            },
        ]

        try:
            response = chat_qwen(client, messages)
            score = _parse_score(response)
        except Exception:
            score = 0

        result: dict = {
            "job_data": job,
            "user_id": username,
            "score": score,
            "auto_bookmarked": False,
        }

        # 4. Auto-bookmark jika skor >= threshold
        if score >= MATCH_THRESHOLD:
            req: dict = {
                "user_id": username,
                "job_id": job_id,
                "source": "auto",
                "score": score,
            }
            success = send_bookmark_request(req)
            result["auto_bookmarked"] = success
            if success:
                bookmarked_count += 1
                print(f"  [v] [{i+1}/{len(jobs)}] {title[:40]:40s} -> skor {score:3d} -> BOOKMARK")
            else:
                print(f"  [x] [{i+1}/{len(jobs)}] {title[:40]:40s} -> skor {score:3d} -> GAGAL BOOKMARK")
        else:
            print(f"      [{i+1}/{len(jobs)}] {title[:40]:40s} -> skor {score:3d} -> skip")

        results.append(result)

    # Ringkasan
    print("\n" + "=" * 60)
    print("  HASIL AUTO-MATCH")
    print("=" * 60)
    print(f"  Total lowongan  : {len(jobs)}")
    print(f"  Dicocokkan      : {len(results)}")
    print(f"  Auto-bookmarked : {bookmarked_count}")
    print(f"  Threshold       : {MATCH_THRESHOLD}")
    print("=" * 60)
