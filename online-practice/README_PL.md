# T7 Online Practice - sonda pamięci

Pierwszy etap przygotowania **Online Practice dla prywatnego Player Match w Tekken 7**.

Ta wersja jest celowo **tylko do odczytu**. Nie zamraża HP, nie zmienia timera i nie zapisuje niczego do pamięci gry. Jej zadaniem jest potwierdzić aktualny build gry oraz znaleźć wiarygodny offset HP bez zgadywania starych adresów z nieaktualnych tabel Cheat Engine.

## Dlaczego tak

Stare mody Online Practice działały przez modyfikację pamięci po obu stronach meczu, ale ich adresy przestały pasować do późniejszego builda gry. Zamiast wklejać stare adresy, sonda:

- sprawdza SHA-256 TekkenGame-Win64-Shipping.exe;
- rozwiązuje struktury P1/P2;
- zapisuje dwa kontrolowane snapshoty;
- szuka tego samego pola HP u obu graczy;
- zapisuje kandydatów do T7OnlinePracticeProbe.log.

Sprawdzony hash używany przez projekt Tekken Accessibility:

7f2b68727023ce9f4554b47f5442dad9dfb38c8a9b778f2aa758469e2c2d60ed

Sonda korzysta z końcowych adresów i rozmiaru struktury znanych dla Tekken 7 jako punktu startowego, ale nie wykonuje zapisu nawet wtedy, gdy hash się zgadza.

## Budowanie

Uruchom x64 Native Tools Command Prompt for VS i wykonaj:

BUILD_X64.bat

Powstanie T7OnlinePracticeProbe.exe.

## Używanie z NVDA

1. Uruchom Tekken 7.
2. Wejdź do zwykłego Practice.
3. Uruchom T7OnlinePracticeProbe.exe.
4. Zresetuj obie postacie do pełnego HP.
5. F5 - snapshot bazowy.
6. Niech P1 dostanie jeden zwykły cios; F6.
7. Niech P2 dostanie jeden zwykły cios; F7.
8. Odczytaj T7OnlinePracticeProbe.log.
9. Esc kończy sondę.

Program wypisuje komunikaty do konsoli i daje sygnał systemowy po F5/F6/F7, więc nie wymaga obserwowania obrazu.

## Kolejny etap

Po potwierdzeniu offsetu HP dokładamy właściwy moduł Private Online Practice:

- zamrożenie HP obu graczy;
- zatrzymanie timera;
- identyczna konfiguracja po obu stronach;
- jawny przełącznik ON/OFF;
- blokadę uruchamiania w Ranked/Tournament, o ile stan trybu da się wiarygodnie rozpoznać;
- współpracę z istniejącym Tekken Accessibility i osobno z VibrationMax/Ki Charge.

Nie planujemy funkcji dających przewagę w publicznym Ranked/Tournament. Celem jest trening ze znajomą osobą, która również świadomie uruchamia ten sam moduł.