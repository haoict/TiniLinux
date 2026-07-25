#!/usr/bin/env python3
"""
yt-play.py - Download and play YouTube videos/audio with caching and library.

Usage:
    yt-play.py <video_url> [format]
    yt-play.py --library

Format options:
    video  (default) - mp4 360p  (format code 18)
    audio            - m4a 48k   (format code 139)

Examples:
    yt-play.py https://www.youtube.com/watch?v=abcd1234
    yt-play.py https://www.youtube.com/watch?v=abcd1234 audio
    yt-play.py --library
"""

import base64
import json
import re
import stat
import sys
import subprocess
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

WORK_DIR     = Path.home() / ".yt-dlp"
DOWNLOAD_DIR = WORK_DIR / "downloaded"
LIBRARY_FILE = WORK_DIR / "library.json"
YT_DLP_BIN   = Path("/usr/local/bin/yt-dlp")
YT_DLP_URL   = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp"

# yt-dlp --list-formats $VIDEO_URL
# 18 - mp4 360p
# 139 - m4a 48k
FORMAT_MAP = {
    "video": "18",    # mp4 360p
    "audio": "139",   # m4a 48k
}
DEFAULT_FORMAT = "video"


# ---------------------------------------------------------------------------
# Directory / binary bootstrap
# ---------------------------------------------------------------------------

def ensure_dirs() -> None:
    DOWNLOAD_DIR.mkdir(parents=True, exist_ok=True)


def ensure_yt_dlp() -> None:
    if YT_DLP_BIN.exists():
        return
    print(f"[yt-play] yt-dlp not found – downloading to {YT_DLP_BIN} …")
    try:
        urllib.request.urlretrieve(YT_DLP_URL, YT_DLP_BIN)
        current = stat.S_IMODE(YT_DLP_BIN.stat().st_mode)
        YT_DLP_BIN.chmod(current | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        print("[yt-play] yt-dlp downloaded successfully.")
    except Exception as exc:
        sys.exit(f"[yt-play] Failed to download yt-dlp: {exc}")


# ---------------------------------------------------------------------------
# URL / ID helpers
# ---------------------------------------------------------------------------

# YouTube video IDs are 11 characters: alphanumeric, hyphen, underscore
_VIDEO_ID_RE = re.compile(r"^[A-Za-z0-9_-]{11}$")


def extract_video_id(url_or_id: str) -> tuple[str, str]:
    """
    Accept either a bare video ID or any YouTube URL variant.
    Returns (video_id, canonical_url).
    """
    # Bare ID: exactly 11 chars
    if _VIDEO_ID_RE.match(url_or_id):
        vid = url_or_id
        return vid, f"https://www.youtube.com/watch?v={vid}"

    parsed = urllib.parse.urlparse(url_or_id)

    if parsed.netloc in ("youtu.be", "www.youtu.be"):
        vid = parsed.path.lstrip("/").split("/")[0]
        if vid:
            return vid, url_or_id

    match = re.search(r"/shorts/([A-Za-z0-9_-]+)", parsed.path)
    if match:
        return match.group(1), url_or_id

    qs = urllib.parse.parse_qs(parsed.query)
    if "v" in qs:
        return qs["v"][0], url_or_id

    sys.exit(
        f"[yt-play] Could not extract video ID from: {url_or_id}\n"
        "Expected a video ID (11 chars), a URL with '?v=<id>', '/shorts/<id>', or 'youtu.be/<id>'."
    )


# ---------------------------------------------------------------------------
# Metadata / thumbnail
# ---------------------------------------------------------------------------

def fetch_metadata(video_url: str) -> dict:
    """
    Use yt-dlp --dump-json to retrieve video metadata.
    Returns a dict with at least: title, upload_date, thumbnails.
    """
    print("[yt-play] Fetching metadata …")
    result = subprocess.run(
        [str(YT_DLP_BIN), "--dump-json", "--no-download", video_url],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"[yt-play] Warning: could not fetch metadata: {result.stderr.strip()}")
        return {}
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        print("[yt-play] Warning: failed to parse metadata JSON.")
        return {}


def pick_smallest_thumbnail(thumbnails: list[dict]) -> str | None:
    """
    From the list of thumbnail dicts returned by yt-dlp, pick the one with
    the smallest resolution (width * height) that has a URL.
    Falls back to the last entry if no dimensions are available.
    """
    if not thumbnails:
        return None

    with_size = [t for t in thumbnails if t.get("width") and t.get("height") and t.get("url")]
    if with_size:
        smallest = min(with_size, key=lambda t: t["width"] * t["height"])
        return smallest["url"]

    # fallback: first thumbnail with a URL
    for t in thumbnails:
        if t.get("url"):
            return t["url"]
    return None


def thumbnail_to_base64(url: str) -> str | None:
    """Download a thumbnail URL and return a data-URI base64 string."""
    try:
        with urllib.request.urlopen(url, timeout=10) as resp:
            raw = resp.read()
            content_type = resp.headers.get_content_type() or "image/jpeg"
        b64 = base64.b64encode(raw).decode()
        return f"data:{content_type};base64,{b64}"
    except Exception as exc:
        print(f"[yt-play] Warning: could not download thumbnail: {exc}")
        return None


def build_library_entry(video_id: str, video_url: str, meta: dict) -> dict:
    """Construct the library record for one video (format-agnostic)."""
    title       = meta.get("title") or video_id
    upload_date = meta.get("upload_date") or ""        # YYYYMMDD or ""

    # Normalise upload_date -> ISO 8601 date string (YYYY-MM-DD)
    if re.fullmatch(r"\d{8}", upload_date):
        upload_date = f"{upload_date[:4]}-{upload_date[4:6]}-{upload_date[6:]}"

    thumbnail_b64 = None
    thumbnails    = meta.get("thumbnails") or []
    thumb_url     = pick_smallest_thumbnail(thumbnails)
    if thumb_url:
        thumbnail_b64 = thumbnail_to_base64(thumb_url)

    return {
        "url"           : video_url,
        "title"         : title,
        "upload_date"   : upload_date,
        "thumbnail_b64" : thumbnail_b64,
        "downloaded_at" : datetime.now(timezone.utc).isoformat(),
    }


# ---------------------------------------------------------------------------
# Library persistence
# ---------------------------------------------------------------------------

def load_library() -> dict:
    """Load library.json → dict keyed by video_id."""
    if not LIBRARY_FILE.exists():
        return {}
    try:
        return json.loads(LIBRARY_FILE.read_text(encoding="utf-8"))
    except Exception as exc:
        print(f"[yt-play] Warning: could not read library: {exc}")
        return {}


def save_library(library: dict) -> None:
    try:
        LIBRARY_FILE.write_text(
            json.dumps(library, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )
    except Exception as exc:
        print(f"[yt-play] Warning: could not save library: {exc}")


def upsert_library(video_id: str, video_url: str, meta: dict | None) -> None:
    """Add or update the library entry for this video (keyed by video_id)."""
    library = load_library()

    if video_id in library:
        # Already recorded – just refresh the downloaded_at timestamp.
        library[video_id]["downloaded_at"] = datetime.now(timezone.utc).isoformat()
    else:
        if not meta:
            print("[yt-play] Warning: no metadata available, skipping library update.")
            return
        library[video_id] = build_library_entry(video_id, video_url, meta)
        title = library[video_id]["title"]
        print(f'[yt-play] Library updated: "{title}"')

    save_library(library)


# ---------------------------------------------------------------------------
# Library display
# ---------------------------------------------------------------------------

def print_library() -> None:
    library = load_library()
    if not library:
        print("[yt-play] Library is empty.")
        return

    print(f"\n{'─' * 48}")
    print(f"{'ID':<12} {'DATE':<11} TITLE")
    print(f"{'─' * 48}")
    for key, entry in sorted(library.items(), key=lambda x: x[1].get("downloaded_at", "")):
        title = entry.get("title", "?").encode("ascii", errors="replace").decode("ascii")
        title = title if len(title) <= 23 else title[:20] + "..."
        date  = entry.get("upload_date", "?")
        print(f"{key:<12} {date:<11} {title}")
    print(f"{'─' * 48}")
    print(f"  {len(library)} item(s)  •  {LIBRARY_FILE}\n")


# ---------------------------------------------------------------------------
# Cache / download / play
# ---------------------------------------------------------------------------

def cached_file(video_id: str, fmt_code: str) -> Path | None:
    candidate = DOWNLOAD_DIR / f"{video_id}.{fmt_code}"
    return candidate if candidate.exists() else None


def download_video(video_url: str, fmt_code: str, dest: Path) -> None:
    # Note: yt-dlp will skip re-downloading if the file already exists, but we check ourselves first to avoid the overhead of spawning yt-dlp for cache hits.    print(f"[yt-play] Downloading format {fmt_code} → {dest} …")
    # E.g: yt-dlp -f 18 -o /path/to/abcd1234.18 https://www.youtube.com/watch?v=abcd1234
    result = subprocess.run([
        str(YT_DLP_BIN), "-f", fmt_code, "-o", str(dest), video_url,
    ])
    if result.returncode != 0:
        sys.exit(f"[yt-play] yt-dlp exited with code {result.returncode}.")
    print(f"[yt-play] Download complete: {dest}")


def play(file_path: Path) -> None:
    print(f"[yt-play] Playing: {file_path}")
    result = subprocess.run(["mpv", "--fs", str(file_path)])
    if result.returncode != 0:
        print(f"[yt-play] mpv exited with code {result.returncode}.")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    # --library flag
    if sys.argv[1] in ("--library", "-l"):
        print_library()
        return

    video_url   = sys.argv[1]
    format_name = sys.argv[2] if len(sys.argv) >= 3 else DEFAULT_FORMAT

    if format_name not in FORMAT_MAP:
        sys.exit(
            f"[yt-play] Unknown format '{format_name}'. "
            f"Valid options: {', '.join(FORMAT_MAP)}"
        )

    fmt_code = FORMAT_MAP[format_name]

    ensure_dirs()
    ensure_yt_dlp()

    video_id, video_url = extract_video_id(video_url)
    print(f"[yt-play] Video ID : {video_id}")
    print(f"[yt-play] Format   : {format_name} (code {fmt_code})")

    # Only fetch metadata when this video_id is not yet in the library
    library = load_library()
    if video_id in library:
        meta = None
        print("[yt-play] Metadata skipped (already in library).")
    else:
        meta = fetch_metadata(video_url)

    cached = cached_file(video_id, fmt_code)
    if cached:
        print("[yt-play] Cache hit – skipping download.")
        upsert_library(video_id, video_url, meta)
        play(cached)
    else:
        dest = DOWNLOAD_DIR / f"{video_id}.{fmt_code}"
        download_video(video_url, fmt_code, dest)
        upsert_library(video_id, video_url, meta)
        play(dest)


if __name__ == "__main__":
    main()
