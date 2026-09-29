#!/usr/bin/env python3
"""
Henter Overleaf-projektet som ZIP og lægger det i mappen DEST i repoet.
Virker på Windows og Mac og kræver kun Python 3 (ingen ekstra pakker).

Brug (fra roden af repoet):
    python sync_overleaf.py      (Windows)
    python3 sync_overleaf.py     (Mac)

Scriptet spørger efter din Overleaf-cookie, når det kører.
Den gemmes ikke nogen steder.
"""
import io
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from datetime import datetime
from pathlib import Path

HOST = "https://overleaf.trit.au.dk"
PROJECT_ID = "6a9fc168660ee8a35677b63a"
DEST = Path("rapport")  # Mappen bliver overskrevet, så brug en mappe kun til rapporten


def load_cookie() -> str:
    """Spørger efter cookien i terminalen (input vises ikke, ligesom et password)."""
    cookie = input("Indsæt værdien af overleaf.sid-cookien: ").strip().strip("'\"")
    if not cookie:
        sys.exit("Fejl: Ingen cookie indtastet.")
    # Tillad både ren værdi og hele 'navn=værdi'
    if "=" not in cookie:
        cookie = f"overleaf.sid={cookie}"
    return cookie


def download_zip(cookie: str) -> bytes:
    url = f"{HOST}/project/{PROJECT_ID}/download/zip"
    req = urllib.request.Request(url, headers={"Cookie": cookie})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            return resp.read()
    except Exception as e:
        sys.exit(f"Fejl under download: {e}")


def git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], capture_output=True, text=True)


def main() -> None:
    if not Path(".git").exists():
        sys.exit("Fejl: Kør scriptet fra roden af jeres Git-repo.")

    print("\nGå ind og find din session cookie på https://overleaf.trit.au.dk/project/6a9fc168660ee8a35677b63a: \n\n1. Åbn udviklerværktøjer \n2. Gå til application fanen \n3. Under Storage->Cookies: Åbn https://overleaf.trit.au.dk/ \n4. Kopier den value som tilhører overleaf.sid\n")

    data = download_zip(load_cookie())

    # Hvis cookien er udløbet, får vi login-siden i stedet for en ZIP
    if not zipfile.is_zipfile(io.BytesIO(data)):
        sys.exit("\nDownload fejlede. Cookien er nok udløbet, så hent en ny fra browseren.")

    if DEST.exists():
        shutil.rmtree(DEST)
    DEST.mkdir(parents=True)
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        zf.extractall(DEST)

    git("add", str(DEST))
    if git("diff", "--cached", "--quiet", "--", str(DEST)).returncode == 0:
        print("\nIngen ændringer i rapporten.")
        return

    msg = f"Sync rapport fra Overleaf ({datetime.now():%Y-%m-%d %H:%M})"
    result = git("commit", "-m", msg)
    if result.returncode != 0:
        sys.exit(f"\nCommit fejlede:\n{result.stderr}")
    print("\nCommittet. Kør 'git push' for at dele det.")


if __name__ == "__main__":
    main()
