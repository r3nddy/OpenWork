"""
LokerID Scraper
Scraper lowongan kerja dari loker.id dengan format output konsisten.

Install:
    uv add playwright playwright-stealth python-dotenv
    uv run playwright install chromium
"""

import asyncio
import json
import os
import re
from pathlib import Path
from urllib.parse import quote_plus

try:
    from dotenv import load_dotenv

    load_dotenv()
except ImportError:
    pass

try:
    from playwright.async_api import async_playwright
except ImportError:
    print("[ERROR]  uv add playwright playwright-stealth python-dotenv")
    print("        uv run playwright install chromium")
    raise SystemExit(1)

try:
    from playwright_stealth import Stealth

    HAS_STEALTH = True
except ImportError:
    HAS_STEALTH = False

try:
    from shared.skema import DEFAULT_MAX_PAGE
except Exception:
    DEFAULT_MAX_PAGE = 3

from scrapers.utils import normalize_job_data, random_delay, scroll_page

BASE_URL = "https://www.loker.id"
MAX_PAGE = int(os.getenv("MAX_PAGE", str(DEFAULT_MAX_PAGE)))


def get_source_name() -> str:
    return "lokerid"


def build_url(keyword: str, location: str, page: int) -> str:
    """
    Bangun URL pencarian LokerID.
    """
    q = quote_plus(keyword.strip()) if keyword else ""
    lok = quote_plus(location.strip()) if location else ""
    
    url = f"{BASE_URL}/cari-lowongan-kerja?q={q}"
    if lok:
        url += f"&lokasi={lok}"
    
    if page > 1:
        # Menambahkan page
        url += f"&page={page}"
        
    return url


def extract_id_from_href(href: str) -> str:
    if not href:
        return ""
    # Contoh href loker.id: /lowongan-kerja/admin-sosial-media-dan-ecommerce.html
    match = re.search(r"/lowongan-kerja/([^/.]+)", href)
    if match:
        return match.group(1)
    parts = [p for p in href.split("/") if p]
    return parts[-1] if parts else ""


async def scrape_dom(page):
    """Ambil data lowongan dari card listing pada DOM."""
    selectors = [
        ".job-box",
        ".job-list",
        "article",
        ".search-result-item",
        "a[href*='/lowongan-kerja/']",
    ]

    for selector in selectors:
        try:
            await page.wait_for_selector(selector, timeout=7000)
            break
        except Exception:
            continue

    return await page.evaluate(
        """() => {
            const cards = Array.from(document.querySelectorAll(".job-box, .job-list, article, .search-result-item, div.row.mb-3"));
            const seen = new Set();
            const rows = [];

            const pickText = (root, selectors) => {
                for (const s of selectors) {
                    const el = root.querySelector(s);
                    if (el && el.textContent) {
                        const val = el.textContent.trim();
                        if (val) return val;
                    }
                }
                return "N/A";
            };

            const normalizeHref = (href) => {
                if (!href) return "";
                if (href.startsWith("http://") || href.startsWith("https://")) return href;
                if (href.startsWith("/")) return href;
                return "/" + href;
            };

            for (const card of cards) {
                const link =
                    card.querySelector("a[href*='/lowongan-kerja/']") ||
                    card.querySelector("h3 a") ||
                    card.querySelector("a.job-title");
                if (!link) continue;

                const href = normalizeHref(link.getAttribute("href") || "");
                const title = (link.textContent || "").trim() || pickText(card, ["h3", "h2", ".title", ".job-title"]);
                if (!href || !title || title === "N/A") continue;
                if (seen.has(href)) continue;
                seen.add(href);

                rows.push({
                    href,
                    title,
                    company: pickText(card, [".company", ".company-name", "strong", "a[href*='/perusahaan/']"]),
                    location: pickText(card, [".location", ".job-location", "i.fa-map-marker + span", ".text-muted"]),
                    salary: pickText(card, [".salary", ".job-salary", "i.fa-money + span"]),
                    work_type: pickText(card, [".type", ".job-type", "span.badge"]),
                    posted_date: pickText(card, [".date", ".posted", ".text-muted"]),
                    teaser: pickText(card, [".description", ".excerpt", "p"]),
                });
            }
            return rows;
        }"""
    )


async def scrape(keyword: str, location: str, max_page: int) -> list[dict]:
    all_jobs = []
    consecutive_empty = 0

    pw_cm = async_playwright()
    if HAS_STEALTH:
        pw_cm = Stealth().use_async(pw_cm)

    async with pw_cm as p:
        browser = await p.chromium.launch(
            headless=False,
            channel="chrome",
            args=["--no-sandbox", "--disable-blink-features=AutomationControlled"],
        )
        context = await browser.new_context(viewport={"width": 1280, "height": 800}, locale="id-ID")
        page = await context.new_page()

        print(f"[SEARCH] Mencari '{keyword}'" + (f" di '{location}'" if location else ""))
        print(f"  Halaman: 1-{max_page}\n")

        for pg in range(1, max_page + 1):
            url = build_url(keyword, location, pg)
            print(f"  [PAGE {pg}] {url}")
            try:
                await page.goto(url, wait_until="domcontentloaded", timeout=45000)
            except Exception as e:
                print(f"    [ERROR] Gagal buka halaman: {e}")
                continue

            await random_delay(1.5, 3.0)
            await scroll_page(page)

            try:
                raw_jobs = await scrape_dom(page)
            except Exception as e:
                print(f"    [ERROR] Gagal parse DOM: {e}")
                raw_jobs = []

            page_jobs = []
            if raw_jobs:
                for raw in raw_jobs:
                    raw["id"] = extract_id_from_href(raw.get("href", ""))
                    page_jobs.append(normalize_job_data(raw, BASE_URL, source=get_source_name()))
                print(f"    [OK] {len(page_jobs)} lowongan ditemukan")
                consecutive_empty = 0
            else:
                consecutive_empty += 1
                print("    [WARN] Tidak ada data pada halaman ini")
                if pg == 1:
                    try:
                        dbg = await page.evaluate(
                            """() => ({
                                url: location.href,
                                title: document.title,
                                cardCount: document.querySelectorAll(".job-box, article, .job-list").length,
                                lowonganLinks: document.querySelectorAll("a[href*='/lowongan-kerja/']").length,
                                bodyPreview: (document.body?.innerText || "").slice(0, 300)
                            })"""
                        )
                        print(f"    [DEBUG] {json.dumps(dbg, ensure_ascii=False, indent=2)}")
                    except Exception:
                        pass

            if page_jobs:
                all_jobs.extend(page_jobs)
                print(f"    Total: {len(all_jobs)} lowongan terkumpul")

            if consecutive_empty >= 2:
                print("    [STOP] 2 halaman kosong berturut-turut, hentikan scraping.")
                break

            if pg < max_page:
                d = await random_delay(1.0, 2.5)
                print(f"    [WAIT] {d:.1f}s...")

        await browser.close()

    return all_jobs
