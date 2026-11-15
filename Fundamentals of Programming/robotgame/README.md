# Robotgame

A C++ robot programming puzzle game developed for the **Fundamentals of Programming** course at Tsinghua University. Players write instruction sequences to transform each level's input into the required output, using inboxes, outboxes, storage, arithmetic, and jumps.

## Versions

- **Online judge submission** (`main.cpp`): Reads a level number and an instruction sequence from standard input, then reports success, failure, or an instruction error.
- **Interactive version 1** (`src/robotgame_v1.cpp`): A console game with animated execution, pause, single-step playback, and restart controls.
- **Interactive version 2** (`src/robotgame_v2.cpp`): A console game that prints the game state after each instruction to simulate animation.

## Structure

```text
robotgame/
├── main.cpp                 # Online judge submission
├── robotgame.pdf            # Original project PDF
├── src/
│   ├── robotgame_v1.cpp     # Interactive version with playback controls
│   └── robotgame_v2.cpp     # Sequential state display version
└── README.md
```

## Build and Run

The source files use UTF-8 encoding and require **C++11 or later**. The interactive versions require **Windows**, since they use Windows console APIs; version 1 also uses `conio.h`.

Build the online judge submission from this project directory:

```bash
g++ -std=c++11 main.cpp -o robotgame_oj
```

Example input for the first level:

```text
1
4
inbox
outbox
inbox
outbox
```

Expected output:

```text
Success
```

For the interactive versions, open a Windows terminal and run from `src/`:

```powershell
cd src
g++ -std=c++11 robotgame_v1.cpp -o robotgame_v1.exe
g++ -std=c++11 robotgame_v2.cpp -o robotgame_v2.exe
.\robotgame_v1.exe
# Or: .\robotgame_v2.exe
```

Version 1 stores level progress in `level.txt`; version 2 uses `level2.txt`. After completing a level, press Enter to return to level selection and save progress. For file input, place the input file in `src/` and enter its filename when prompted. Additional controls are shown in the game interface.

The original project PDF is available as [`robotgame.pdf`](robotgame.pdf).
