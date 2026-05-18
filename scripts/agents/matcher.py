"""
matcher.py — Analisis Kecocokan Profil (AI Agent)
Baca file lowongan yang ditandai + profil user,
lalu minta Ai menganalisis kecocokan.
"""

import json
import textwrap
import re
import os
from pathlib import Path
from agents.client import buat_client_openrouter, chat_qwen


# System Prompt
SYSTEM_PROMPT_MATCH = (
    "Kamu adalah konsultan karier AI yang ahli. Tugasmu adalah menganalisis "
    "kecocokan antara profil pencari kerja dengan lowongan kerja yang diminati. "
    "Gunakan bahasa Indonesia. Berikan analisis yang mencakup:\n"
    "1. Ringkasan profil pencari kerja\n"
    "2. Untuk setiap lowongan, analisis tingkat kecocokan (rendah/sedang/tinggi) berdasarkan:\n"
    "   - Kecocokan skills\n"
    "   - Kecocokan lokasi\n"
    "   - Kecocokan gaji (ekspektasi vs yang ditawarkan)\n"
    "   - Kecocokan bidang\n"
    "3. Ranking lowongan dari yang paling cocok ke yang kurang cocok\n"
    "4. Saran pengembangan skill yang perlu ditingkatkan\n"
    "Jawab dengan format yang rapi dan mudah dibaca."
)


# Helper: Print dengan Word Wrap
def print_wrapped(text: str, width: int = 76, indent: str = "  ") -> None:
    """Mencetak teks dengan pembungkus otomatis (word wrap), indentasi cerdas, dan hapus markdown."""
    # Bersihkan markdown teks tebal (**) dan judul (###)
    text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
    text = re.sub(r'^\s*#+\s+', '', text, flags=re.MULTILINE)
    
    for line in text.split('\n'):
        if not line.strip():
            print()
            continue
            
        # Ekstrak spasi bawaan dari AI untuk menjaga hierarki (misal sub-bullet)
        leading_spaces = len(line) - len(line.lstrip())
        base_indent = indent + " " * leading_spaces
        stripped_line = line.lstrip()
        
        # Deteksi apakah baris ini diawali dengan bullet (- , * , 1. )
        match = re.match(r'^((?:-|\*|\d+\.)\s+)', stripped_line)
        if match:
            bullet_len = len(match.group(1))
            sub_indent = base_indent + " " * bullet_len
        else:
            sub_indent = base_indent
            
        # Bungkus teks dengan batas margin dan indentasi yang dihitung
        wrapped_lines = textwrap.wrap(
            stripped_line, 
            width=width, 
            initial_indent=base_indent, 
            subsequent_indent=sub_indent
        )
        for w in wrapped_lines:
            print(w)


# Helper: Build Profile Summary
def _build_profile_summary(profile: dict) -> str:
    """Bangun ringkasan profil user untuk dikirim ke LLM."""
    skills_list = profile.get('skills', [])
    skills_formatted = []

    if isinstance(skills_list, list):
        for s in skills_list:
            if isinstance(s, dict):
                name = s.get('name', 'N/A')
                level = s.get('level', 'N/A')
                skills_formatted.append(f"{name} ({level})")
            else:
                skills_formatted.append(str(s))
        skills_str = ", ".join(skills_formatted) if skills_formatted else "N/A"
    else:
        skills_str = str(skills_list)

    return (
        f"Nama: {profile.get('name', 'N/A')}\n"
        f"Lokasi: {profile.get('location', 'N/A')}\n"
        f"Ekspektasi Gaji: {profile.get('salaryExpectation', 'N/A')}\n"
        f"Skills: {skills_str}"
    )


def _build_job_summaries_for_match(jobs: list[dict], max_jobs: int = 30) -> str:
    """Bangun ringkasan lowongan untuk analisis kecocokan."""
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

        summary = (
            f"{i}. {title} — {company}\n"
            f"   Lokasi: {location} | Gaji: {salary}\n"
            f"   Deskripsi: {desc}"
        )
        if skills:
            summary += f"\n   Skills dibutuhkan: {skills}"
        if field:
            summary += f"\n   Bidang: {field}"
        summaries.append(summary)

    return "\n\n".join(summaries)


def _clear_screen() -> None:
    """Bersihkan layar terminal (cross-platform)."""
    os.system('cls' if os.name == 'nt' else 'clear')


def _tampilkan_banner_tanya_jawab() -> None:
    """Tampilkan ulang banner header tanya jawab setelah layar dibersihkan."""
    print("+==========================================================+")
    print("|         TANYA JAWAB ANALISIS KECOCOKAN - OPENWORK        |")
    print("+==========================================================+")
    print("|  Mode    : Tanya Jawab Analisis Kecocokan                |")
    print("|  Keluar  : Ketik 'exit' atau 'quit' untuk keluar         |")
    print("|  Clear   : Ketik '/clear' atau '/cls' untuk bersihkan    |")
    print("+==========================================================+")
    print()


# Main Function
def run_match(lowongan_path: str, profile_path: str) -> None:
    """Baca file lowongan ditandai + profil user, lalu minta Qwen menganalisis kecocokan."""
    try:
        # Baca lowongan ditandai
        lowongan_file = Path(lowongan_path)
        if not lowongan_file.exists():
            print(f"[ERROR] File lowongan tidak ditemukan: {lowongan_path}")
            return

        with open(lowongan_file, "r", encoding="utf-8") as f:
            jobs = json.load(f)

        if not isinstance(jobs, list) or len(jobs) == 0:
            print("[ERROR] File lowongan kosong atau bukan array.")
            return

        # Baca profil user
        profile_file = Path(profile_path)
        if not profile_file.exists():
            print(f"[ERROR] File profil tidak ditemukan: {profile_path}")
            return

        with open(profile_file, "r", encoding="utf-8") as f:
            profile = json.load(f)

        if not isinstance(profile, dict):
            # Jika masih berupa array (kasus darurat), ambil elemen pertama
            if isinstance(profile, list) and len(profile) > 0:
                profile = profile[0]
            else:
                print("[ERROR] File profil tidak valid (harus object JSON).")
                return

        print(f"[OK] Membaca {len(jobs)} lowongan ditandai")
        print(f"[OK] Membaca profil: {profile.get('name', 'N/A')}")
        print("[INFO] Mengirim ke AI untuk analisis kecocokan...\n")

        profile_text = _build_profile_summary(profile)
        jobs_text = _build_job_summaries_for_match(jobs)

        client = buat_client_openrouter()

        messages = [
            {"role": "system", "content": SYSTEM_PROMPT_MATCH},
            {
                "role": "user",
                "content": (
                    f"Analisis kecocokan profil saya dengan {len(jobs)} lowongan yang saya tandai:\n\n"
                    f"=== PROFIL SAYA ===\n{profile_text}\n\n"
                    f"=== LOWONGAN YANG DITANDAI ===\n{jobs_text}"
                ),
            },
        ]

        result = chat_qwen(client, messages)

        print("=" * 80)
        print("  ANALISIS KECOCOKAN PROFIL")
        print("=" * 80)
        print()
        print_wrapped(result, width=80, indent="  ")
        print()
        print("=" * 80)

        # Mode tanya jawab lanjutan
        messages.append({"role": "assistant", "content": result})

        print("\nKamu bisa bertanya lebih lanjut tentang kecocokan ini.")
        print("Ketik 'exit' / 'quit' untuk keluar.\n")

        while True:
            try:
                user_input = input("You> ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\nSampai jumpa!")
                return

            if not user_input:
                print('\033[1A\033[2K', end='')
                continue
            if user_input.lower() in {"/clear", "/cls"}:
                _clear_screen()
                _tampilkan_banner_tanya_jawab()
                continue
            if user_input.lower() in {"exit", "quit"}:
                print("Sampai jumpa!")
                return

            messages.append({"role": "user", "content": user_input})
            assistant_text = chat_qwen(client, messages)
            messages.append({"role": "assistant", "content": assistant_text})

            print("\nOpenWork AI> ")
            print_wrapped(assistant_text, width=80, indent="  ")
            print()

    except json.JSONDecodeError as e:
        print(f"[ERROR] Gagal parsing JSON: {e}")
    except Exception as e:
        print(f"[ERROR] {e}")
