"""
Utility functions yang dipakai oleh semua scraper modules.
"""

import asyncio
import random


async def random_delay(min_sec: float = 2.0, max_sec: float = 4.0) -> None:
    """Tunggu random delay untuk menghindari deteksi bot."""
    delay = random.uniform(min_sec, max_sec)
    await asyncio.sleep(delay)
    return delay


async def scroll_page(page, max_scrolls: int = 15, pause: float = 1.0) -> None:
    """Scroll halaman secara iteratif dari atas ke bawah untuk memuat lazy-loaded content."""
    try:
        for _ in range(max_scrolls):
            # Scroll ke bawah sebesar 1 layar penuh
            await page.evaluate("window.scrollBy(0, window.innerHeight * 0.9)")
            await asyncio.sleep(pause)
            
            # Cek apakah sudah sampai paling bawah
            reached_bottom = await page.evaluate(
                "Math.ceil(window.scrollY + window.innerHeight) >= document.body.scrollHeight"
            )
            if reached_bottom:
                break
    except Exception:
        pass


import hashlib
from datetime import datetime, timezone

def deduplicate_cross_source(jobs: list[dict]) -> tuple[list[dict], int]:
    """
    Hapus duplikat dari daftar lowongan gabungan.
    Key dedup: title.lower() + company.lower()
    Jika duplikat: gabungkan field "source" menjadi list.
    Return: (unique_jobs, total_duplikat_dihapus)
    """
    seen = {}
    duplikat_count = 0
    
    for job in jobs:
        title = str(job.get("title", "")).strip().lower()
        company = str(job.get("company", "")).strip().lower()
        
        # Fallback key kalau title/company kosong atau "N/A"
        if not title or title == "n/a" or not company or company == "n/a":
            # Gunakan ID asli sebagai fallback yang sangat spesifik
            key = job.get("id", "")
            if not key:
                import uuid
                key = str(uuid.uuid4()) # Jangan sampai meniban data sah yang tidak punya key
        else:
            key = f"{title}|{company}"
            
        if key in seen:
            duplikat_count += 1
            # Gabungkan source
            existing_job = seen[key]
            existing_source = existing_job.get("source")
            new_source = job.get("source")
            
            if not isinstance(existing_source, list):
                existing_source = [existing_source] if existing_source else []
                existing_job["source"] = existing_source
                
            if isinstance(new_source, list):
                for ns in new_source:
                    if ns and ns not in existing_job["source"]:
                        existing_job["source"].append(ns)
            else:
                if new_source and new_source not in existing_job["source"]:
                    existing_job["source"].append(new_source)
        else:
            seen[key] = job
            
    return list(seen.values()), duplikat_count


def extract_salary(salary_str: str) -> tuple[int | None, int | None]:
    """Ekstrak angka dari string gaji secara kasar."""
    if not salary_str or salary_str == "N/A":
        return None, None
        
    import re
    # Hapus semua karakter kecuali digit, minus, dan titik/koma
    # Ini sangat kasar, idealnya pakai NLP/regex yg lebih cerdas,
    # tapi untuk boilerplate sudah cukup.
    numbers = re.findall(r'\d+(?:\.\d+)?', salary_str.replace(',', ''))
    if not numbers:
        return None, None
        
    nums = [int(float(n)) for n in numbers]
    if len(nums) >= 2:
        return min(nums), max(nums)
    elif len(nums) == 1:
        return nums[0], nums[0]
    return None, None


def normalize_job_data(raw: dict, base_url: str = "", source: str = "unknown") -> dict:
    """Normalisasi data mentah dari scraping ke format standar gabungan."""
    href = raw.get("href", "")
    
    if href.startswith("http"):
        url = href
    elif href.startswith("/"):
        url = f"{base_url}{href}"
    else:
        url = f"{base_url}/{href}" if href else ""
        
    # Buat hash dari URL sebagai ID (kecuali raw sudah punya id unik, tapi user minta hash url)
    job_id = hashlib.md5(url.encode('utf-8')).hexdigest() if url else raw.get("id", "")
    
    salary_str = raw.get("salary", "N/A")
    sal_min, sal_max = extract_salary(salary_str)
    
    # Kumpulkan field-field
    title = raw.get("title", "N/A")
    company = raw.get("company", "N/A")
    location = raw.get("location", "N/A")
    teaser = raw.get("teaser", "N/A")
    
    # Ambil field work_type jika tidak ada field khusus
    field = raw.get("field") or raw.get("work_type", "N/A")
    
    scraped_at = datetime.now(timezone.utc).isoformat()
    
    return {
        "id": job_id,
        "title": title,
        "company": company,
        "location": location,
        "salary_min": sal_min,
        "salary_max": sal_max,
        "field": field,
        "description": teaser,
        "requirements": [], # Karena biasanya di halaman detail, biarkan list kosong
        "url": url,
        "source": source,
        "scraped_at": scraped_at
    }
