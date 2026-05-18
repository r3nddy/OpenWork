"""
Jobstreet Scraper
Workflow:
    1. Pertama kali: buka browser → login manual → simpan state.json
    2. Selanjutnya: pakai state.json → langsung scrape (tanpa login)

Install (via uv):
    uv add playwright playwright-stealth python-dotenv
    uv run playwright install chromium
"""

import asyncio
import json
import os
from pathlib import Path

try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass

try:
    from playwright.async_api import async_playwright
except ImportError:
    print("[ERROR]  uv add playwright playwright-stealth python-dotenv")
    print("        playwright install chromium")
    exit(1)

try:
    from playwright_stealth import Stealth
    HAS_STEALTH = True
except ImportError:
    HAS_STEALTH = False

from shared.skema import (
    HASIL_JOBSTREET_FILE,
    STATE_FILE,
    BASE_URL_JOBSTREET,
    DEFAULT_MAX_PAGE,
)
from scrapers.utils import random_delay, scroll_page, normalize_job_data


# Config
MAX_PAGE = int(os.getenv("MAX_PAGE", str(DEFAULT_MAX_PAGE)))
BASE_URL = BASE_URL_JOBSTREET


def get_source_name() -> str:
    return "jobstreet"


def build_url(keyword, location, page):
    """Bangun URL pencarian Jobstreet."""
    kw = keyword.strip().replace(" ", "-").lower()
    path = f"/id/{kw}-jobs/in-{location.strip().replace(' ','-').lower()}" if location else f"/id/{kw}-jobs"
    return f"{BASE_URL}{path}?pg={page}" if page > 1 else f"{BASE_URL}{path}"


async def scrape_dom(page):
    """Scrape job cards dari DOM."""
    try:
        # Tunggu cards muncul
        try:
            await page.wait_for_selector('article[data-automation="normalJobCard"]', timeout=10000)
        except Exception:
            try:
                await page.wait_for_selector('a[data-automation="jobTitle"]', timeout=5000)
            except Exception:
                return []

        return await page.evaluate("""() => {
            const results = [];
            const cards = document.querySelectorAll('article[data-automation="normalJobCard"], article');
            for (const card of cards) {
                const titleEl = card.querySelector('a[data-automation="jobTitle"]');
                if (!titleEl) continue;
                const title = titleEl.textContent?.trim() || 'N/A';
                const href = titleEl.getAttribute('href') || '';
                const titleId = titleEl.getAttribute('id') || '';
                let id = titleId.startsWith('job-title-') ? titleId.replace('job-title-', '') : '';
                if (!id) { const m = href.match(/job\\/(\\w+)/); id = m ? m[1] : ''; }
                const get = (sel) => card.querySelector(sel)?.textContent?.trim() || 'N/A';
                results.push({
                    id, title, href,
                    company: get('a[data-automation="jobCompany"]'),
                    location: get('[data-automation="jobLocation"]'),
                    salary: get('[data-automation="jobSalary"]'),
                    teaser: get('[data-automation="jobShortDescription"]'),
                    posted_date: get('[data-automation="jobListingDate"]'),
                });
            }
            return results;
        }""")
    except Exception as e:
        print(f"    [ERROR] DOM: {e}")
        return []


# Main
async def scrape(keyword: str, location: str, max_page: int) -> list[dict]:
    """Fungsi utama untuk scraping Jobstreet."""
    all_jobs = []

    # Setup playwright (with stealth if available)
    pw_cm = async_playwright()
    if HAS_STEALTH:
        pw_cm = Stealth().use_async(pw_cm)

    state_path = str(STATE_FILE)

    async with pw_cm as p:
        has_state = STATE_FILE.exists()

        # STEP 1: Login sekali, simpan state
        if not has_state:
            print("=" * 60)
            print("  PERTAMA KALI — LOGIN DIPERLUKAN")
            print("=" * 60)
            print()
            print("  Browser akan terbuka.")
            print("  Login ke Jobstreet (Google/Email/dll).")
            print("  Setelah login berhasil, state disimpan otomatis.")
            print("  Run berikutnya TIDAK perlu login lagi.")
            print()

            browser = await p.chromium.launch(
                headless=False,
                channel="chrome",
                args=["--no-sandbox", "--disable-blink-features=AutomationControlled"],
            )
            context = await browser.new_context(
                viewport={"width": 1280, "height": 800},
                locale="id-ID",
            )
            page = await context.new_page()

            await page.goto("https://id.jobstreet.com/",
                          wait_until="domcontentloaded", timeout=30000)

            # Tunggu user selesai login (max 180 detik)
            print("[WAIT] Menunggu kamu login di browser...")
            logged_in = False
            for i in range(0, 180, 3):
                await asyncio.sleep(3)
                try:
                    url = page.url.lower()
                    if all(kw not in url for kw in ["login", "signin", "oauth", "accounts.google"]):
                        logged_in = True
                        break
                except Exception:
                    pass
                if i % 15 == 0 and i > 0:
                    print(f"  ... masih menunggu ({i}s/180s)")

            if logged_in:
                await context.storage_state(path=state_path)
                print(f"[OK] Login berhasil! State disimpan ke '{state_path}'")
                print("[INFO] Jalankan script lagi untuk mulai scraping.")
            else:
                print("[ERROR] Timeout login. Coba jalankan ulang.")

            await browser.close()
            return []

        # STEP 2: Scrape dengan state yang sudah ada
        print(f"[OK] Memuat state dari '{state_path}'")

        browser = await p.chromium.launch(
            headless=False,
            channel="chrome",
            args=["--no-sandbox", "--disable-blink-features=AutomationControlled"],
        )
        context = await browser.new_context(
            storage_state=state_path,
            viewport={"width": 1280, "height": 800},
            locale="id-ID",
        )
        page = await context.new_page()

        # Verifikasi state masih valid
        print("[INFO] Verifikasi session...")
        first_url = build_url(keyword, location, 1)
        await page.goto(first_url,
                       wait_until="networkidle", timeout=45000)
        await asyncio.sleep(2)

        if "login" in page.url.lower() or "signin" in page.url.lower():
            print("[WARN] Session expired! Menghapus state lama...")
            STATE_FILE.unlink(missing_ok=True)
            print("[INFO] Jalankan script lagi untuk login ulang.")
            await browser.close()
            return []

        print("[OK] Session valid!")

        # Scraping loop
        print(f"\n[SEARCH] Mencari '{keyword}'" + (f" di '{location}'" if location else ""))
        print(f"  Halaman: 1-{max_page}\n")

        try:
            for pg in range(1, max_page + 1):
                url = build_url(keyword, location, pg)
                print(f"  [PAGE {pg}] {url}")

                # Navigate
                try:
                    await page.goto(url, wait_until="networkidle", timeout=45000)
                except Exception as e:
                    print(f"    [ERROR] {e}")
                    continue

                await random_delay(2, 4)

                # Scroll lazy load
                await scroll_page(page)

                # Check redirect
                try:
                    if "login" in page.url.lower():
                        print("    [WARN] Session expired mid-scrape!")
                        STATE_FILE.unlink(missing_ok=True)
                        break
                except Exception:
                    break

                # ── Extract data (DOM scraping) ─────────────────────────
                page_jobs = []
                raw_dom = await scrape_dom(page)
                if raw_dom:
                    for r in raw_dom:
                        page_jobs.append(normalize_job_data(r, BASE_URL, source=get_source_name()))
                    print(f"    [OK] {len(page_jobs)} lowongan ditemukan")

                # Debug on page 1 if nothing found
                if not page_jobs and pg == 1:
                    try:
                        info = await page.evaluate("""() => ({
                            url: location.href,
                            articleCount: document.querySelectorAll('article').length,
                            jobTitleCount: document.querySelectorAll('a[data-automation="jobTitle"]').length,
                            dataAutomations: [...new Set([...document.querySelectorAll('[data-automation]')].map(e => e.getAttribute('data-automation')))],
                            bodyPreview: document.body?.innerText?.substring(0, 300),
                        })""")
                        print(f"    [DEBUG] {json.dumps(info, indent=2, ensure_ascii=False)}")
                    except Exception:
                        pass

                if page_jobs:
                    all_jobs.extend(page_jobs)
                    print(f"    Total: {len(all_jobs)} lowongan terkumpul")
                else:
                    print("    [WARN] Tidak ada data")

                if pg < max_page:
                    d = await random_delay(2, 4)
                    print(f"    [WAIT] {d:.1f}s...")

        except Exception as e:
            if "closed" in str(e).lower():
                print(f"\n[WARN] Browser ditutup oleh user")
            else:
                print(f"\n[ERROR] {e}")

        try:
            await browser.close()
        except Exception:
            pass

    return all_jobs
