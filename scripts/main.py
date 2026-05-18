"""
OpenWork Services — Orchestrator Entry Point
=============================================
Satu entry point untuk semua layanan Python.
Dipanggil dari C++ via subprocess.

Usage:
    uv run python main.py scrape                    → Jalankan scraper (tanpa auto-match)
    uv run python main.py scrape <username>         → Jalankan scraper + auto-match + auto-bookmark
    uv run python main.py chat                      → Chat REPL dengan Qwen
    uv run python main.py analyze <json_path>       → Analisis lowongan
    uv run python main.py match <jobs_path> <profile_path>  → Kecocokan profil
    uv run python main.py auto-match <username>     → Auto-match lowongan dengan profil user
"""

import sys
import os

if sys.stdout and hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

# Tambahkan services/ ke sys.path agar import modul berfungsi
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def print_usage():
    """Tampilkan cara pakai."""
    print("=" * 60)
    print("  OpenWork Services")
    print("=" * 60)
    print()
    print("  Usage:")
    print("    uv run python main.py <command> [args]")
    print()
    print("  Commands:")
    print("    scrape                     Jalankan scraper (semua sumber)")
    print("    scrape <username>          Scrape + auto-match + auto-bookmark")
    print("    chat                       Chat REPL dengan AI (Qwen)")
    print("    analyze <json_path>        Analisis lowongan dari file JSON")
    print("    match <jobs> <profile>     Analisis kecocokan profil vs lowongan")
    print("    auto-match <username>      Auto-match lowongan hasil scrape dengan profil")
    print()
    print("  Examples:")
    print('    uv run python main.py scrape')
    print('    uv run python main.py scrape user1')
    print('    uv run python main.py chat')
    print('    uv run python main.py analyze "../data/scrape/merged_data.json"')
    print('    uv run python main.py match "../data/bookmarked_jobs.json" "../data/profile.json"')
    print('    uv run python main.py auto-match user1')
    print()


def main():
    if len(sys.argv) < 2:
        print_usage()
        return

    command = sys.argv[1].lower()

    if command == "scrape":
        from scrapers.scraper_manager import run_manager
        run_manager()

        # Jika username diberikan, lanjutkan ke auto-match
        if len(sys.argv) >= 3:
            username = sys.argv[2]
            print(f"\n[AUTO] Melanjutkan ke auto-match untuk user: {username}")
            from agents.auto_matcher import run_auto_match
            run_auto_match(username)

    elif command == "chat":
        from agents.client import run_repl
        run_repl()

    elif command == "analyze":
        if len(sys.argv) < 3:
            print("[ERROR] Missing argument: json_path")
            print("  Usage: uv run python main.py analyze <json_path>")
            return
        from agents.analyst import run_analyze
        run_analyze(sys.argv[2])

    elif command == "match":
        if len(sys.argv) < 4:
            print("[ERROR] Missing arguments: jobs_path, profile_path")
            print("  Usage: uv run python main.py match <jobs_path> <profile_path>")
            return
        from agents.matcher import run_match
        run_match(sys.argv[2], sys.argv[3])

    elif command == "auto-match":
        if len(sys.argv) < 3:
            print("[ERROR] Missing argument: username")
            print("  Usage: uv run python main.py auto-match <username>")
            return
        from agents.auto_matcher import run_auto_match
        run_auto_match(sys.argv[2])

    elif command in {"--help", "-h", "help"}:
        print_usage()

    else:
        print(f"[ERROR] Command tidak dikenal: '{command}'")
        print_usage()


if __name__ == "__main__":
    main()
