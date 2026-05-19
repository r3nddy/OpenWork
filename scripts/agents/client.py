"""
client.py — OpenRouter LLM Client.
Setup dan wrapper untuk komunikasi dengan Qwen via OpenRouter.
Dipakai oleh analyst.py, matcher.py, dan modul agent lainnya.
"""

import os
import re
import textwrap

try:
    from dotenv import load_dotenv
except ImportError:
    pass

try:
    from openai import OpenAI
except ImportError:
    print("[ERROR] Install dulu: uv add openai python-dotenv")
    exit(1)


def _find_and_load_env():
    """Cari dan load .env dari root project."""
    from shared.skema import PROJECT_ROOT

    env_path = PROJECT_ROOT / ".env"
    if env_path.exists():
        load_dotenv(env_path)
    else:
        # Fallback: cari di services/
        from shared.skema import SERVICES_DIR
        env_fallback = SERVICES_DIR / ".env"
        if env_fallback.exists():
            load_dotenv(env_fallback)
        else:
            load_dotenv()  # default behavior


# Load env saat module di-import
_find_and_load_env()


def buat_client_openrouter() -> OpenAI:
    """Buat OpenAI client yang mengarah ke OpenRouter."""
    api_key = os.getenv("OPENROUTER_API_KEY")
    if not api_key:
        raise RuntimeError(
            "API Key tidak ditemukan. Pastikan environment variable "
            "OPENROUTER_API_KEY sudah diset di file .env di root project."
        )
    return OpenAI(
        base_url="https://openrouter.ai/api/v1",
        api_key=api_key,
    )


def chat_qwen(client: OpenAI, messages: list[dict]) -> str:
    """Kirim messages ke Qwen dan return response text."""
    try:
        completion = client.chat.completions.create(
            extra_headers={
                "HTTP-Referer": "http://localhost:3000",
                "X-Title": "OpenWork AI Agent",
            },
            model="qwen/qwen-2.5-72b-instruct",
            messages=messages,
        )

        # Validasi: pastikan choices tidak kosong/None
        if not hasattr(completion, 'choices') or not completion.choices:
            error_detail = getattr(completion, 'error', None)
            if error_detail:
                return f"[API_ERROR] OpenRouter error: {error_detail}"
            return "[API_ERROR] Respons OpenRouter kosong — kemungkinan token limit terlampaui atau server sedang overload."

        content = completion.choices[0].message.content

        # Validasi: pastikan content tidak None
        if content is None:
            return "[API_ERROR] Konten respons AI kosong (None). Coba kurangi ukuran data input."

        return content
    except Exception as e:
        return f"[API_ERROR] {e}"


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

def _clear_screen() -> None:
    """Bersihkan layar terminal (cross-platform)."""
    os.system('cls' if os.name == 'nt' else 'clear')


def _tampilkan_banner_chat() -> None:
    """Tampilkan ulang banner header setelah layar dibersihkan."""
    print("+==========================================================+")
    print("|              CHAT AI AGENT - OPENWORK                    |")
    print("+==========================================================+")
    print("|  Mode    : Chat Bebas dengan AI                          |")
    print("|  Tips    : Tanya lowongan, skill, atau saran karier      |")
    print("|  Keluar  : Ketik 'exit' atau 'quit' di dalam chat        |")
    print("|  Reset   : Ketik '/reset' untuk hapus konteks chat       |")
    print("|  Clear   : Ketik '/clear' atau '/cls' untuk bersihkan    |")
    print("+==========================================================+")
    print()


def run_repl() -> None:
    """Mode chat interaktif (REPL) dengan AI Agent."""
    client = buat_client_openrouter()

    messages: list[dict] = [
        {
            "role": "system",
            "content": (
                "Kamu adalah asisten AI yang membantu menjawab pertanyaan "
                "pengguna dengan jelas dan ringkas."
            ),
        }
    ]



    while True:
        try:
            user_input = input("You> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nSampai jumpa!")
            return

        if not user_input:
            print('\033[1A\033[2K', end='')
            continue
        if user_input.lower() in {"exit", "quit"}:
            print("Sampai jumpa!")
            return
        if user_input.lower() == "/reset":
            messages = messages[:1]
            _clear_screen()
            _tampilkan_banner_chat()
            print("  [OK] Konteks percakapan di-reset. Sesi baru dimulai.\n")
            continue
        if user_input.lower() in {"/clear", "/cls"}:
            _clear_screen()
            _tampilkan_banner_chat()
            continue

        messages.append({"role": "user", "content": user_input})
        assistant_text = chat_qwen(client, messages)
        messages.append({"role": "assistant", "content": assistant_text})

        print("\nOpenWork AI> ")
        print_wrapped(assistant_text, width=80, indent="  ")
        print()
