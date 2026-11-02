#!/usr/bin/env python3
"""Validate a TEKKEN 7 Access prerelease ZIP before publishing it."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import zipfile

ROOT = Path.cwd().resolve()
PACKAGE_NAME = re.compile(r"TekkenAccessibility-(\d+\.\d+\.\d+-PL\d+)\.zip\Z")


def fail(reason):
    raise SystemExit("BŁĄD PUBLIKACJI: " + reason)


def select_package():
    if os.environ.get("TRIGGER_EVENT") == "workflow_dispatch":
        return os.environ.get("REQUESTED_PATH", "").strip()

    before = os.environ.get("BASE_SHA", "").strip()
    head = os.environ.get("HEAD_SHA", "").strip()
    if not re.fullmatch(r"[0-9a-f]{40}", head):
        fail("Nieprawidłowy commit HEAD.")
    if re.fullmatch(r"0{40}", before):
        command = ["git", "diff-tree", "--no-commit-id", "--name-only", "-r", head, "--", "release-packages/"]
    elif re.fullmatch(r"[0-9a-f]{40}", before):
        command = ["git", "diff", "--name-only", "--diff-filter=AM", before, head, "--", "release-packages/"]
    else:
        fail("Brak identyfikatora wcześniejszego commita.")
    changed = subprocess.check_output(command, text=True).splitlines()
    packages = [name for name in changed if name.startswith("release-packages/") and name.endswith(".zip")]
    if len(packages) != 1:
        fail(f"Jedna zmiana ZIP na publikację; wykryto: {packages!r}.")
    return packages[0]


def validate():
    rel = select_package()
    if not rel.startswith("release-packages/") or Path(rel).parent.as_posix() != "release-packages":
        fail("Paczka musi być bezpośrednio w release-packages/.")
    match = PACKAGE_NAME.fullmatch(Path(rel).name)
    if not match:
        fail("Wymagana nazwa: TekkenAccessibility-X.Y.Z-PLn.zip")
    tag = match.group(1)
    base = tag.split("-PL", 1)[0]
    package = (ROOT / rel).resolve()
    if not package.is_relative_to(ROOT / "release-packages") or not package.is_file():
        fail(f"Plik nie istnieje lub ścieżka jest nieprawidłowa: {rel}")
    if package.stat().st_size > 100_000_000:
        fail("Paczka przekracza 100 MB.")
    notes_rel = f"release-notes/{tag}.md"
    notes = ROOT / notes_rel
    if not notes.is_file() or not notes.read_text(encoding="utf-8-sig").strip():
        fail(f"Brak polskiego opisu wydania: {notes_rel}")

    prefix = f"TekkenAccessibility-{base}/"
    required = {
        prefix + "TekkenAccessibility.dll",
        prefix + "TekkenAccessVersion.txt",
        prefix + "CHANGELOG_PL.txt",
    }
    with zipfile.ZipFile(package) as archive:
        all_info = archive.infolist()
        if not all_info or sum(item.file_size for item in all_info) > 150_000_000:
            fail("Pusta paczka lub zbyt duża suma plików.")
        names = [item.filename for item in all_info]
        if len(set(n.lower() for n in names)) != len(names):
            fail("Powtarzające się nazwy plików w ZIP.")
        for entry in all_info:
            p = Path(entry.filename)
            if p.is_absolute() or ".." in p.parts or "\\" in entry.filename or not entry.filename.startswith(prefix):
                fail(f"Nieprawidłowa ścieżka w ZIP: {entry.filename}")
            if entry.flag_bits & 1:
                fail("Zaszyfrowany ZIP nie jest obsługiwany.")
        missing = required - set(names)
        if missing:
            fail(f"Brak wymaganych plików: {sorted(missing)}")
        if archive.testzip() is not None:
            fail("CRC ZIP nie przeszło kontroli.")
        if archive.read(prefix + "TekkenAccessVersion.txt").decode("utf-8-sig").strip() != base:
            fail("Wewnętrzny numer wersji nie zgadza się z nazwą paczki.")
        dll = archive.read(prefix + "TekkenAccessibility.dll")
        if len(dll) < 100_000 or dll[:2] != b"MZ":
            fail("Brak prawidłowego pliku DLL Windows.")
        if tag == "3.20.30-PL1":
            for phrase in ("Wyszukaj sesję", "Wyszukiwanie sesji", "Liczba miejsc"):
                if phrase.encode("utf-16le") not in dll:
                    fail(f"W DLL brakuje poprawki PL1: {phrase}")
    sha256 = hashlib.sha256(package.read_bytes()).hexdigest()
    print(f"Kontrola ZIP PASS: {rel} ({package.stat().st_size} B)")
    print(f"Tag: {tag} | SHA256: {sha256}")
    outputs = os.environ.get("GITHUB_OUTPUT")
    if outputs:
        with open(outputs, "a", encoding="utf-8") as file:
            file.write(f"release_tag={tag}\npackage_path={rel}\nnotes_path={notes_rel}\nsha256={sha256}\n")


if __name__ == "__main__":
    try:
        validate()
    except (OSError, zipfile.BadZipFile, subprocess.CalledProcessError, UnicodeError) as exc:
        fail(str(exc))
