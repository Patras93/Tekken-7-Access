#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "bcrypt.lib")

namespace {
constexpr wchar_t kProcessName[] = L"TekkenGame-Win64-Shipping.exe";
constexpr std::uintptr_t kP1PtrOffset = 0x34DF630;
constexpr std::uintptr_t kPlayerStructSize = 0x3670;
constexpr std::uintptr_t kCurrentMoveOffset = 0x350;
constexpr char kExpectedExeSha256[] =
    "7f2b68727023ce9f4554b47f5442dad9dfb38c8a9b778f2aa758469e2c2d60ed";

struct Snapshot {
    std::vector<unsigned char> p1;
    std::vector<unsigned char> p2;
    bool valid = false;
};

std::ofstream gLog("T7OnlinePracticeProbe.log", std::ios::out | std::ios::trunc);

void Log(const std::string& s) {
    std::cout << s << std::endl;
    if (gLog) {
        gLog << s << std::endl;
        gLog.flush();
    }
}

std::string Hex(std::uintptr_t v) {
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << v;
    return os.str();
}

DWORD FindProcessId() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, kProcessName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return pid;
}

std::uintptr_t FindModuleBase(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    std::uintptr_t base = 0;

    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, kProcessName) == 0) {
                base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
                break;
            }
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
    return base;
}

bool GetProcessImagePath(HANDLE process, std::wstring& out) {
    DWORD size = 32768;
    std::vector<wchar_t> buf(size);
    if (!QueryFullProcessImageNameW(process, 0, buf.data(), &size)) return false;
    out.assign(buf.data(), size);
    return true;
}

bool Sha256File(const std::wstring& path, std::string& outHex) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    NTSTATUS st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (st < 0) return false;

    DWORD objLen = 0, hashLen = 0, cb = 0;
    st = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
                           reinterpret_cast<PUCHAR>(&objLen), sizeof(objLen), &cb, 0);
    if (st < 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return false;
    }
    st = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH,
                           reinterpret_cast<PUCHAR>(&hashLen), sizeof(hashLen), &cb, 0);
    if (st < 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return false;
    }

    std::vector<unsigned char> obj(objLen);
    std::vector<unsigned char> digest(hashLen);

    st = BCryptCreateHash(alg, &hash, obj.data(), static_cast<ULONG>(obj.size()),
                          nullptr, 0, 0);
    if (st < 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return false;
    }

    std::vector<char> buf(1 << 20);
    while (f) {
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const auto got = f.gcount();
        if (got > 0) {
            st = BCryptHashData(hash, reinterpret_cast<PUCHAR>(buf.data()),
                                static_cast<ULONG>(got), 0);
            if (st < 0) {
                BCryptDestroyHash(hash);
                BCryptCloseAlgorithmProvider(alg, 0);
                return false;
            }
        }
    }

    st = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (st < 0) return false;

    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (unsigned char b : digest) os << std::setw(2) << static_cast<unsigned>(b);
    outHex = os.str();
    return true;
}

template <typename T>
bool Read(HANDLE process, std::uintptr_t address, T& value) {
    SIZE_T done = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address),
                             &value, sizeof(T), &done) &&
           done == sizeof(T);
}

bool ReadBlock(HANDLE process, std::uintptr_t address,
               std::vector<unsigned char>& data, std::size_t size) {
    data.resize(size);
    SIZE_T done = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address),
                             data.data(), size, &done) &&
           done == size;
}

std::int32_t I32(const std::vector<unsigned char>& b, std::size_t off) {
    std::int32_t v = 0;
    std::memcpy(&v, b.data() + off, sizeof(v));
    return v;
}

float F32(const std::vector<unsigned char>& b, std::size_t off) {
    float v = 0.0f;
    std::memcpy(&v, b.data() + off, sizeof(v));
    return v;
}

struct Candidate {
    std::size_t offset;
    std::int32_t p1Before;
    std::int32_t p1After;
};

struct FloatCandidate {
    std::size_t offset;
    float p1Before;
    float p1After;
};

std::vector<Candidate> BuildIntCandidates(const Snapshot& base, const Snapshot& p1Hit) {
    std::vector<Candidate> out;
    for (std::size_t off = 0; off + 4 <= base.p1.size(); off += 4) {
        const auto a1 = I32(base.p1, off);
        const auto b1 = I32(p1Hit.p1, off);
        const auto a2 = I32(base.p2, off);
        const auto b2 = I32(p1Hit.p2, off);

        const auto drop = static_cast<long long>(a1) - static_cast<long long>(b1);
        if (a1 > 0 && a1 <= 100000 && b1 >= 0 && drop > 0 && drop <= 10000 &&
            std::llabs(static_cast<long long>(a2) - static_cast<long long>(b2)) <= 1) {
            out.push_back({off, a1, b1});
        }
    }
    return out;
}

std::vector<FloatCandidate> BuildFloatCandidates(const Snapshot& base, const Snapshot& p1Hit) {
    std::vector<FloatCandidate> out;
    for (std::size_t off = 0; off + 4 <= base.p1.size(); off += 4) {
        const float a1 = F32(base.p1, off);
        const float b1 = F32(p1Hit.p1, off);
        const float a2 = F32(base.p2, off);
        const float b2 = F32(p1Hit.p2, off);

        if (!std::isfinite(a1) || !std::isfinite(b1) ||
            !std::isfinite(a2) || !std::isfinite(b2)) {
            continue;
        }
        const float drop = a1 - b1;
        if (a1 > 0.0f && a1 <= 100000.0f && b1 >= 0.0f &&
            drop > 0.001f && drop <= 10000.0f &&
            std::fabs(a2 - b2) <= 0.001f) {
            out.push_back({off, a1, b1});
        }
    }
    return out;
}

void ReportFiltered(const Snapshot& base,
                    const std::vector<Candidate>& ints,
                    const std::vector<FloatCandidate>& floats,
                    const Snapshot& p2Hit) {
    Log("=== Kandydaci HP po uderzeniu P2 ===");

    int count = 0;
    for (const auto& c : ints) {
        const auto p2Before = I32(base.p2, c.offset);
        const auto p2After = I32(p2Hit.p2, c.offset);
        const auto drop = static_cast<long long>(p2Before) - static_cast<long long>(p2After);
        if (p2Before > 0 && p2After >= 0 && drop > 0 && drop <= 10000) {
            std::ostringstream os;
            os << "INT offset=" << Hex(c.offset)
               << " P1 " << c.p1Before << " -> " << c.p1After
               << " P2 " << p2Before << " -> " << p2After;
            Log(os.str());
            ++count;
        }
    }

    for (const auto& c : floats) {
        const float p2Before = F32(base.p2, c.offset);
        const float p2After = F32(p2Hit.p2, c.offset);
        const float drop = p2Before - p2After;
        if (std::isfinite(p2Before) && std::isfinite(p2After) &&
            p2Before > 0.0f && p2After >= 0.0f &&
            drop > 0.001f && drop <= 10000.0f) {
            std::ostringstream os;
            os << std::fixed << std::setprecision(3)
               << "FLOAT offset=" << Hex(c.offset)
               << " P1 " << c.p1Before << " -> " << c.p1After
               << " P2 " << p2Before << " -> " << p2After;
            Log(os.str());
            ++count;
        }
    }

    if (count == 0) {
        Log("Brak pewnych kandydatow. Powtorz probe: F5 przy pelnym HP, potem P1 dostaje cios + F6, P2 dostaje cios + F7.");
    } else {
        Log("Wynik zapisano w T7OnlinePracticeProbe.log. Nie wlaczamy jeszcze zadnego zapisu do pamieci.");
    }
}

} // namespace

int main() {
    SetConsoleTitleW(L"T7 Online Practice Probe - read only");
    Log("T7 Online Practice Probe - tryb tylko do odczytu.");
    Log("Nie uzywaj tego do Ranked/Tournament. Cel: prywatny Player Match z druga osoba.");

    const DWORD pid = FindProcessId();
    if (!pid) {
        Log("Nie znaleziono TekkenGame-Win64-Shipping.exe. Uruchom Tekken 7 i sprobuj ponownie.");
        return 2;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process) {
        Log("Nie mozna otworzyc procesu Tekken 7 do odczytu.");
        return 3;
    }

    const auto base = FindModuleBase(pid);
    if (!base) {
        Log("Nie znaleziono bazy modulu TekkenGame-Win64-Shipping.exe.");
        CloseHandle(process);
        return 4;
    }

    std::wstring imagePath;
    if (GetProcessImagePath(process, imagePath)) {
        std::string sha;
        if (Sha256File(imagePath, sha)) {
            Log("SHA-256 EXE: " + sha);
            if (_stricmp(sha.c_str(), kExpectedExeSha256) == 0) {
                Log("Build zgodny z wersja sprawdzona przez Tekken Accessibility.");
            } else {
                Log("UWAGA: hash EXE jest inny niz sprawdzony build. Sonda pozostaje tylko do odczytu.");
            }
        } else {
            Log("Nie udalo sie policzyc SHA-256 EXE.");
        }
    }

    std::uint64_t p1 = 0;
    const auto p1PtrAddr = base + kP1PtrOffset;
    if (!Read(process, p1PtrAddr, p1) || p1 == 0) {
        Log("Nie udalo sie rozwiazac wskaznika P1 pod " + Hex(p1PtrAddr) + ".");
        Log("Offset P1 wymaga ponownego sprawdzenia dla tego builda.");
        CloseHandle(process);
        return 5;
    }

    const std::uintptr_t p1Addr = static_cast<std::uintptr_t>(p1);
    const std::uintptr_t p2Addr = p1Addr + kPlayerStructSize;

    std::int32_t move1 = 0, move2 = 0;
    Read(process, p1Addr + kCurrentMoveOffset, move1);
    Read(process, p2Addr + kCurrentMoveOffset, move2);

    Log("Module base: " + Hex(base));
    Log("P1: " + Hex(p1Addr) + "  P2: " + Hex(p2Addr));
    Log("Current move P1=" + std::to_string(move1) + " P2=" + std::to_string(move2));
    Log("");
    Log("Procedura namierzania HP:");
    Log("1. Wejdz do Practice i zresetuj obie postacie do pelnego HP.");
    Log("2. Nacisnij F5 - zapis bazowy.");
    Log("3. P1 dostaje jeden zwykly cios. Nacisnij F6.");
    Log("4. P2 dostaje jeden zwykly cios. Nacisnij F7.");
    Log("ESC konczy program.");

    Snapshot baseline, p1Hit, p2Hit;
    std::vector<Candidate> intCandidates;
    std::vector<FloatCandidate> floatCandidates;

    bool prevF5 = false, prevF6 = false, prevF7 = false, prevEsc = false;

    for (;;) {
        const bool f5 = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        const bool f6 = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
        const bool f7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
        const bool esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

        if (f5 && !prevF5) {
            baseline.valid =
                ReadBlock(process, p1Addr, baseline.p1, kPlayerStructSize) &&
                ReadBlock(process, p2Addr, baseline.p2, kPlayerStructSize);
            Log(baseline.valid ? "F5: zapis bazowy OK." : "F5: blad odczytu struktur.");
            MessageBeep(baseline.valid ? MB_OK : MB_ICONERROR);
        }

        if (f6 && !prevF6) {
            if (!baseline.valid) {
                Log("Najpierw nacisnij F5 przy pelnym HP.");
                MessageBeep(MB_ICONWARNING);
            } else {
                p1Hit.valid =
                    ReadBlock(process, p1Addr, p1Hit.p1, kPlayerStructSize) &&
                    ReadBlock(process, p2Addr, p1Hit.p2, kPlayerStructSize);
                if (p1Hit.valid) {
                    intCandidates = BuildIntCandidates(baseline, p1Hit);
                    floatCandidates = BuildFloatCandidates(baseline, p1Hit);
                    Log("F6: kandydaci po ciosie P1: INT=" +
                        std::to_string(intCandidates.size()) +
                        " FLOAT=" + std::to_string(floatCandidates.size()));
                    Log("Teraz niech P2 dostanie jeden cios i nacisnij F7.");
                    MessageBeep(MB_OK);
                } else {
                    Log("F6: blad odczytu struktur.");
                    MessageBeep(MB_ICONERROR);
                }
            }
        }

        if (f7 && !prevF7) {
            if (!baseline.valid || !p1Hit.valid) {
                Log("Najpierw wykonaj F5 i F6.");
                MessageBeep(MB_ICONWARNING);
            } else {
                p2Hit.valid =
                    ReadBlock(process, p1Addr, p2Hit.p1, kPlayerStructSize) &&
                    ReadBlock(process, p2Addr, p2Hit.p2, kPlayerStructSize);
                if (p2Hit.valid) {
                    ReportFiltered(baseline, intCandidates, floatCandidates, p2Hit);
                    MessageBeep(MB_OK);
                } else {
                    Log("F7: blad odczytu struktur.");
                    MessageBeep(MB_ICONERROR);
                }
            }
        }

        if (esc && !prevEsc) break;

        prevF5 = f5;
        prevF6 = f6;
        prevF7 = f7;
        prevEsc = esc;
        Sleep(25);
    }

    CloseHandle(process);
    Log("Koniec.");
    return 0;
}
