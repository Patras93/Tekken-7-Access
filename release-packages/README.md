# Publikowanie nowych wersji Tekken 7 Access

**GitHub Actions tworzy Release automatycznie** po dodaniu nowego ZIP-a do tego folderu na gałęzi `main`. Nie trzeba ręcznie tworzyć tagu ani Release.

1. Przygotuj gotowy ZIP z DLL, np. `TekkenAccessibility-3.20.30-PL1.zip`.
2. Dodaj opis `release-notes/3.20.30-PL1.md` (jeśli jeszcze go nie ma).
3. Wejdź do folderu `release-packages` na GitHubie, wybierz **Add file → Upload files**, prześlij ZIP i zatwierdź do `main`.
4. W zakładce **Actions** otwórz `Tekken 7 Access - automatyczny Release`. Jeśli weryfikacja zakończy się powodzeniem, gotowa paczka pojawi się w **Releases** jako wydanie TEST.

Nazwy paczek: `TekkenAccessibility-X.Y.Z-PLn.zip`. Wewnątrz ZIP musi znajdować się folder `TekkenAccessibility-X.Y.Z` zawierający `TekkenAccessibility.dll`, `CHANGELOG_PL.txt` i `TekkenAccessVersion.txt` (z wartością `X.Y.Z`). Każde nowe wydanie wymaga nowego tagu — workflow nie podmienia istniejących Releases.

Możesz także uruchomić **Actions → Tekken 7 Access - automatyczny Release → Run workflow**, podając ścieżkę ZIP-a już znajdującego się w repozytorium.

Jeśli GitHub zgłosi brak prawa do publikacji Release, właściciel repozytorium może sprawdzić **Settings → Actions → General → Workflow permissions → Read and write permissions**. Workflow żąda `contents: write`, ale ustawienia organizacji lub repozytorium mogą to ograniczać.

**Ważne:** to automatyzacja publikacji gotowej DLL, a nie kompilator C++. Test integralności nie zastępuje testu z NVDA w samej grze. Nie dodawaj tu źródeł DLL ani archiwów innych projektów.
