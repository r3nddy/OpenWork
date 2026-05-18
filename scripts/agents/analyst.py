"""
analyst.py — Analisis Lowongan Kerja (AI Agent)
Baca file JSON hasil scraping, kirim ke Qwen untuk analisis.
"""

import json
from pathlib import Path
from agents.client import buat_client_openrouter, chat_qwen


# ASCII Table Formatter
def format_laporan_tabel(raw_text: str) -> None:
    """Parse JSON dari AI dan tampilkan sebagai tabel ASCII yang rapi."""
    W = 90  # lebar dalam tabel

    def line_eq():
        print("=" * (W + 2))

    def line_dash():
        print(f"|{'─' * W}|")

    def row(text: str):
        print(f"| {text:<{W - 1}}|")

    def row_num(no: str, text: str):
        print(f"| {no:<4} | {text:<{W - 8}}|")

    # Coba parse JSON dari response AI
    try:
        cleaned = raw_text.strip()
        if cleaned.startswith("```"):
            cleaned = cleaned.split("\n", 1)[1] if "\n" in cleaned else cleaned[3:]
            if cleaned.endswith("```"):
                cleaned = cleaned[:-3]
            cleaned = cleaned.strip()

        data = json.loads(cleaned)
    except (json.JSONDecodeError, Exception):
        print("=" * 60)
        print("  ANALISIS AI AGENT")
        print("=" * 60)
        print()
        print(raw_text)
        print()
        print("=" * 60)
        return

    bulan = data.get("bulan", "2025")

    # Header
    line_eq()
    title = f"LAPORAN PASAR KERJA — {bulan}"
    row(title.center(W - 1))
    line_eq()

    # Skills
    row("SKILL TERPANAS")
    line_dash()
    for i, s in enumerate(data.get("skills", [])[:5], 1):
        nama = s.get("nama", "?")
        persen = s.get("persen", 0)
        ket = s.get("keterangan", "")
        txt = f"{nama:<30} ↑ {persen}%"
        if ket:
            txt += f" ({ket})"
        row_num(f"{i}.", txt)
    line_eq()

    # Gaji
    row("RATA-RATA GAJI PER POSISI")
    line_dash()
    for i, g in enumerate(data.get("gaji", [])[:5], 1):
        posisi = g.get("posisi", "?")
        gaji = g.get("gaji", "N/A")
        row_num(f"{i}.", f"{posisi:<50} : {gaji}")
    line_eq()

    # Perusahaan 
    row("PERUSAHAAN PALING AKTIF HIRING")
    line_dash()
    for i, p in enumerate(data.get("perusahaan", [])[:5], 1):
        nama = p.get("nama", "?")
        jumlah = p.get("jumlah", 0)
        row_num(f"{i}.", f"{nama:<40} — {jumlah} posisi terbuka")
    line_eq()

    # Insights
    row("INSIGHT AI")
    line_dash()
    for insight in data.get("insights", [])[:3]:
        print(f"  • {insight}")
    line_eq()


# Build Job Summary
def _build_job_summaries(jobs: list[dict], max_jobs: int = 30) -> str:
    """Bangun ringkasan lowongan untuk dikirim ke LLM."""
    summaries = []
    for i, job in enumerate(jobs[:max_jobs], 1):
        title = job.get('title', 'N/A')
        company = job.get('company', 'N/A')
        location = job.get('location', 'N/A')
        salary_min = job.get('salary_min')
        salary_max = job.get('salary_max')
        salary = "N/A"
        if salary_min is not None or salary_max is not None:
            if salary_min and salary_max:
                salary = f"Rp {salary_min} - {salary_max}"
            elif salary_min:
                salary = f"Rp > {salary_min}"
            elif salary_max:
                salary = f"Rp < {salary_max}"
        else:
            salary = job.get('salary') or job.get('salaryExpectation', 'N/A')
            
        desc = job.get('description') or job.get('teaser') or job.get('desc', 'N/A')
        skills = job.get('skills', '')
        field = job.get('field', '')
        url = job.get('url', '')

        summary = f"{i}. {title} — {company}\n   Lokasi: {location} | Gaji: {salary}"
        if desc:
            summary += f"\n   Deskripsi: {desc}"
        if skills:
            summary += f"\n   Skills: {skills}"
        if field:
            summary += f"\n   Bidang: {field}"
        if url:
            summary += f"\n   URL: {url}"
        summaries.append(summary)

    return "\n\n".join(summaries)


# System Prompt
SYSTEM_PROMPT_ANALYZE = (
    "Kamu adalah konsultan karier AI yang ahli menganalisis pasar kerja. "
    "Tugasmu adalah menganalisis daftar lowongan kerja dan mengembalikan HANYA JSON "
    "(tanpa markdown, tanpa ```json, tanpa penjelasan tambahan).\n\n"
    "Format JSON yang WAJIB dikembalikan:\n"
    "{\n"
    '  "bulan": "Mei 2025",\n'
    '  "skills": [\n'
    '    {"nama": "Python", "persen": 34, "keterangan": "paling banyak diminta"},\n'
    '    {"nama": "SQL", "persen": 28, "keterangan": ""}\n'
    "  ],\n"
    '  "gaji": [\n'
    '    {"posisi": "Backend Engineer", "gaji": "Rp 14.500.000"},\n'
    '    {"posisi": "Frontend Engineer", "gaji": "Rp 12.000.000"}\n'
    "  ],\n"
    '  "perusahaan": [\n'
    '    {"nama": "Tokopedia", "jumlah": 23},\n'
    '    {"nama": "Gojek", "jumlah": 18}\n'
    "  ],\n"
    '  "insights": [\n'
    '    "Tren remote work naik 12% minggu ini.",\n'
    '    "Fintech paling agresif hiring backend.",\n'
    '    "Python dan SQL masih jadi requirement utama."\n'
    "  ]\n"
    "}\n\n"
    "ATURAN:\n"
    "- Kembalikan HANYA JSON valid, tanpa teks lain\n"
    "- Gunakan bahasa Indonesia\n"
    "- skills: top 5 skill paling sering muncul di judul/deskripsi, persen = frekuensi relatif\n"
    "- gaji: rata-rata per jenis posisi (hanya dari yang mencantumkan gaji)\n"
    "- perusahaan: top 5 perusahaan dengan lowongan terbanyak\n"
    "- insights: 3-5 insight spesifik dan actionable berdasarkan data"
)


# Main Function


def run_analyze(json_path: str) -> None:
    """Baca file JSON hasil scraping dan minta Qwen menganalisis."""
    try:
        path = Path(json_path)
        if not path.exists():
            print(f"[ERROR] File tidak ditemukan: {json_path}")
            return

        with open(path, "r", encoding="utf-8") as f:
            jobs = json.load(f)

        if not isinstance(jobs, list) or len(jobs) == 0:
            print("[ERROR] File JSON kosong atau bukan array.")
            return

        print(f"[OK] Membaca {len(jobs)} lowongan dari '{json_path}'")
        print("[INFO] Mengirim ke AI untuk analisis...\n")

        jobs_text = _build_job_summaries(jobs)
        client = buat_client_openrouter()

        messages = [
            {"role": "system", "content": SYSTEM_PROMPT_ANALYZE},
            {
                "role": "user",
                "content": f"Analisis {len(jobs)} lowongan kerja berikut:\n\n{jobs_text}",
            },
        ]

        result = chat_qwen(client, messages)

        # Parse JSON dari AI dan format ke tabel ASCII
        print()
        format_laporan_tabel(result)
        print()

        input("\n  [SELESAI] Tekan Enter untuk kembali ke menu...")

    except json.JSONDecodeError as e:
        print(f"[ERROR] Gagal parsing JSON: {e}")
    except Exception as e:
        print(f"[ERROR] {e}")
