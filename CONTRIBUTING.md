# Contributing to XcX

First off, thank you for considering contributing to XcX!

## Our Philosophy
XcX is a standalone C++17 terminal editor with **no external dependencies** (only libc/libstdc++). To keep it lean and portable, we have a few guidelines:

1. **No New Dependencies:** No curses, no boost, no Python. Just C++17 and the standard library.
2. **Modular Architecture:** Source is split into focused modules (`editor`, `filetree`, `smartbar`, `theme`, `app`). Keep it that way.
3. **Zero Warnings:** All contributions must compile cleanly with `-Wall -Wextra -O2`.
4. **Consistent Style:** Follow the existing naming and formatting conventions in the codebase.

## How to Contribute
- **Bug Reports:** Open an issue with your OS, terminal emulator, and reproduction steps.
- **Feature Requests:** Open an issue to discuss before writing code.
- **Pull Requests:** Ensure clean builds, add a brief description of changes.

## Building
```bash
make
```

No configure step needed. No dependencies to install.
