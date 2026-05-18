"""
bridge.py — Python-to-C++ Bridge.
Modul jembatan untuk memanggil fungsi C++ dari Python via CLI subprocess.
Digunakan oleh matcher dan analyst untuk:
  1. Mengambil profil user dari C++
  2. Mengirim permintaan bookmark ke C++
"""

import json
import subprocess
import sys
from pathlib import Path
from shared.skema import PROJECT_ROOT


def _find_exe() -> str:
    """Cari lokasi OpenWork.exe dari project root."""
    candidates = [
        PROJECT_ROOT / "build" / "bin" / "OpenWork.exe",
        PROJECT_ROOT / "build" / "bin" / "OpenWork",
        PROJECT_ROOT / "build" / "OpenWork.exe",
        PROJECT_ROOT / "build" / "OpenWork",
    ]
    for c in candidates:
        if c.exists():
            return str(c)
    raise FileNotFoundError(
        f"[bridge] OpenWork binary tidak ditemukan. "
        f"Sudah compile? Dicek di: {[str(c) for c in candidates]}"
    )


def get_user_profile(username: str) -> dict | None:
    """
    Ambil profil user dari C++ via bridge-get-profile.
    Returns dict profil atau None jika gagal.
    """
    try:
        exe = _find_exe()
        result = subprocess.run(
            [exe, "bridge-get-profile", username],
            capture_output=True,
            text=True,
            timeout=10,
            cwd=str(PROJECT_ROOT),
        )
        if result.returncode != 0:
            print(f"[bridge] Gagal mengambil profil: {result.stderr.strip()}")
            return None
        return json.loads(result.stdout.strip())
    except FileNotFoundError as e:
        print(f"[bridge] {e}")
        return None
    except json.JSONDecodeError:
        print("[bridge] Gagal parsing JSON profil dari C++.")
        return None
    except subprocess.TimeoutExpired:
        print("[bridge] Timeout saat mengambil profil.")
        return None
    except Exception as e:
        print(f"[bridge] Error: {e}")
        return None


def send_bookmark_request(req: dict) -> bool:
    """
    Kirim permintaan bookmark ke C++ via bridge-add-bookmark.
    Returns True jika berhasil, False jika gagal.
    """
    try:
        exe = _find_exe()
        result = subprocess.run(
            [
                exe,
                "bridge-add-bookmark",
                req["user_id"],
                req["job_id"],
                req.get("source", "auto"),
                str(req.get("score", 0)),
            ],
            capture_output=True,
            text=True,
            timeout=10,
            cwd=str(PROJECT_ROOT),
        )
        if result.returncode != 0:
            print(f"[bridge] Gagal bookmark: {result.stderr.strip()}")
            return False
        # Parse response
        try:
            resp = json.loads(result.stdout.strip())
            return resp.get("status") == "success"
        except json.JSONDecodeError:
            # Cek apakah output mengandung "success"
            return "success" in result.stdout.lower()
    except FileNotFoundError as e:
        print(f"[bridge] {e}")
        return False
    except subprocess.TimeoutExpired:
        print("[bridge] Timeout saat mengirim bookmark.")
        return False
    except Exception as e:
        print(f"[bridge] Error: {e}")
        return False
