# CS2 External Tool
![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows)

Educational project for learning reverse engineering and Windows memory manipulation.

## How it works

The tool runs as a separate process and reads CS2 memory using Windows API (`ReadProcessMemory`).

**Entity resolution:**
CS2 stores entities in a chunked list system. Each chunk holds 512 entities.
To find a pawn by handle index:
- `index >> 9` = chunk number
- `index & 0x1FF` = position inside chunk
- Each entry is 112 bytes

**Process:**
1. Find CS2 window via `FindWindowA`
2. Get PID via `GetWindowThreadProcessId`
3. Open handle via `OpenProcess`
4. Find `client.dll` base via `CreateToolhelp32Snapshot`
5. Read entity list and iterate controllers to find enemies

## Features
- [x] Entity list reading
- [x] HP reading
- [ ] Triggerbot
- [ ] Radar hack
- [ ] Glow ESP
- [ ] RCS
- [ ] Config system (.ini)

## Disclaimer
For educational purposes only. Reverse engineering research.
