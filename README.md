# ═══ XcX Editor v2.0 ═══

> *The terminal editor that doesn't compromise — color themes, a gorgeous block
> cursor, hex mode, git integration, and zero bloat.*

```
g++ -std=c++17 -o xcx main.cpp app.cpp editor.cpp filetree.cpp smartbar.cpp theme.cpp -lncurses -lstdc++fs
```

[![C++17](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![dependency: ncurses](https://img.shields.io/badge/dependency-ncurses-green.svg)](https://invisible-island.net/ncurses/)
[![license: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)
[![lines ~4.8K](https://img.shields.io/badge/source-%7E4.8K%20lines-purple)](.)

---

## What's New in v2.0

| Feature | Description |
|---|---|
| 🎨 **10 Color Themes** | Blue, Red, Green, Gray, RGB, Moon, Ocean, Sunset, Nord, Dracula — switch with `Ctrl+T` |
| 🔲 **Block Cursor** | Bright, visible block cursor with smooth blink. Never lose your place again |
| 📦 **Modular Build** | 9 well-organized `.cpp` + `.h` files with a clean `Makefile`. No more 50KB single-file spaghetti |
| 🎯 **Selection Highlight** | Full visual selection rendering with reverse-video highlight |
| ⌨️ **Ctrl+X / C / V** | Standard cut-copy-paste shortcuts (works with selection) |
| 📄 **Ctrl+A Select All** | One-key select whole buffer |
| 🔍 **Ctrl+W Search** | Quick in-file search (replaces clunky Alt+S) |
| 🖥️ **60fps Render Loop** | Smooth animation, cursor blink, responsive input |

---

## Quick Start

```bash
# Build
make

# Open current directory
./xcx

# Open a specific file
./xcx myfile.py

# Open a project
./xcx /path/to/project
```

---

## Interface

```
┌──────────────────────────────────────────────────────────────────────┐
│ ═══ XcX Editor ═══  main.py*  ══  Blue                               │ ← title + theme name
├──────────────┬─┬──────┬──────────────────────────────────────────────┤
│   EXPLORER   │g│  1   │  import os                                   │
│ ▾ src        │u│  2   │  import sys                                  │
│   main.py    │t│  3   │                                              │
│   utils.py   │t│  4   │  def main():                                 │
│ ▸ tests      │ │  5   │      print("hello")                          │
│              │ │  6   │                                              │
│              │ │  7   │                                              │
├──────────────┴─┴──────┴──────────────────────────────────────────────┤
│ F1 Help  ^O Save  ^W Search  ^K Cut  ^U Paste  ^A Mark  ^L Goto  ^Q │ ← shortcut bar
├──────────────────────────────────────────────────────────────────────┤
│ INSERT  main.py ●    XcX v2.0       Ln 4,9  6L  4sp  py  ◆ Blue     │ ← status bar
└──────────────────────┬───────────────────────────────────────────────┘
                       │
                  ◆ current theme
```

### Elements

1. **Title bar** — filename, modified indicator (`*`), zen/hex mode, active theme
2. **Explorer** — file tree with expandable directories and file icons
3. **Line numbers** — current line highlighted with bold
4. **Editor** — main text area with your beautiful block cursor
5. **Shortcut bar** — nano-style key hints
6. **Status bar** — mode, filename, messages, cursor position, language, theme

---

## Keyboard Shortcuts

### Global

| Key | Action |
|---|---|
| `Ctrl+S` | Save file |
| `Ctrl+Q` | Quit (double-tap if unsaved) |
| `Ctrl+T` | **Switch color theme** (cycles through all 10) |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+F` | Grep across project |
| `Ctrl+P` | Quick open file (fuzzy search) |
| `Ctrl+O` | Outline / symbol search |
| `Ctrl+W` | Search in current file |
| `Ctrl+R` | Find & replace |
| `Ctrl+L` | Go to line number |
| `Ctrl+G` | Help screen |
| `F5` | Focus file explorer |
| `F6` | Focus editor |
| `F11` | Toggle Zen mode |
| `Alt+W` | Change explorer root folder |
| `Alt+X` | Command palette |

### Editor

| Key | Action |
|---|---|
| `↑ ↓ ← →` | Move cursor |
| `Ctrl+← / Ctrl+→` | Word jump |
| `Home` | Smart home (indent ↔ column 0) |
| `End` | End of line |
| `PgUp / PgDn` | Page scroll |
| `Enter` | Newline with auto-indent |
| `Tab` | Insert indent (spaces or tab) |
| `Backspace` | Delete left (or paired bracket) |
| `Delete` | Delete right |
| `Ctrl+X` | Cut (selection or current line) |
| `Ctrl+C` | Copy (selection or current line) |
| `Ctrl+V` | Paste |
| `Ctrl+A` | Select all |
| `Ctrl+K` | Cut line |
| `Ctrl+U` | Paste (alt) |
| `Ctrl+H` | Toggle hex mode |

### Explorer

| Key | Action |
|---|---|
| `↑ / ↓` | Navigate |
| `Enter` | Open file / expand directory |
| `Backspace` | Parent directory |
| `Esc` | Return to editor |

---

## Color Themes

XcX v2.0 ships with **10 carefully crafted color themes**. Press `Ctrl+T` to cycle through them, or use the command palette (`Alt+X` → "theme next" / "theme prev").

| # | Theme | Vibe |
|---|---|---|
| 1 | **Blue** | Classic professional blue (default) |
| 2 | **Red** | Warm ruby/crimson tones |
| 3 | **Green** | Matrix-inspired forest green |
| 4 | **Gray** | Minimal monochrome grayscale |
| 5 | **RGB** | Vibrant rainbow accents |
| 6 | **Moon** | Soft low-contrast night mode |
| 7 | **Ocean** | Deep teal/cyan ocean depths |
| 8 | **Sunset** | Warm orange/purple dusk |
| 9 | **Nord** | Arctic bluish Nord palette |
| 10 | **Dracula** | Dark purple Dracula palette |

The active theme is shown in the title bar and status bar. Theme is applied immediately — no restart needed.

---

## Block Cursor

XcX draws its **own visible block cursor** (the hardware cursor is hidden). You always know exactly where you are:

- **Solid bright block** over the current character
- **Smooth blink** at ~5Hz — pauses on keypress
- **Reverse background** for maximum visibility in any theme
- **Works in hex mode** — highlights the byte position
- **Never lost** — the cursor always stays visible even on empty lines

---

## Features

### Text Editing
- Insert, delete, backspace with bracket auto-close
- Smart indentation detection (tabs vs spaces, auto-detected width)
- Auto-indent on Enter (after `:`, `{`, etc.)
- Bracket auto-delete (backspace deletes matched pair)

### Selection
- `Ctrl+A` selects the entire buffer
- Visual selection is rendered with reverse-video highlight
- Cut/Copy/Paste work with selections
- Cut without selection cuts the current line

### Undo / Redo
- 500-level undo stack with deduplication
- Redo available after undo (cleared on new edits)
- Snapshot-based (full line buffer + cursor state)

### Hex Mode (`Ctrl+H`)
- View file as hexadecimal byte dump
- ASCII representation shown alongside hex
- Cursor highlights the current byte position

### Search & Replace
- **Ctrl+W**: Incremental in-file search with result list
- **Ctrl+R**: Find and replace with two-field input (Tab to switch)
- **Ctrl+F**: Global grep using system `grep` binary
- **Ctrl+O**: Outline view (functions, classes in Python/JS)

### File Management
- **F5**: File explorer with expandable tree
- **Ctrl+P**: Fuzzy file quick-open across the project
- **Alt+W**: Navigate to a different folder
- Backspace in explorer: go to parent directory

### Git Integration
- Status bar shows file modification state
- Command palette: `git status`, `git log`

### Zen Mode (`F11`)
- Hides sidebar, gutter, status bar
- Editor takes the full terminal width
- Minimal distraction mode

### Sticky Scroll
- When scrolled past a function/class definition, the header sticks to the top
- Works for Python (def, class), JS/TS (function, class)

### Command Palette (`Alt+X`)
- Searchable list of all commands
- Commands: save, new, help, outline, theme next/prev, git status/log, hex, zen, undo, redo, select all, exit

---

## Architecture

XcX v2.0 is organized into **9 source files** (7 `.cpp` + 2 `.h` with separate headers):

```
xcx/
├── main.cpp         ← Entry point, ncurses init
├── app.h / app.cpp  ← Main application class, rendering, input dispatch
├── editor.h/.cpp    ← Editor state: buffer, cursor, undo, selection, search
├── filetree.h/.cpp  ← File tree navigation
├── smartbar.h/.cpp  ← Floating input overlay (search, command palette, etc.)
├── theme.h /.cpp    ← Color theme system (10 themes)
├── common.h         ← Shared types, constants, includes
├── Makefile         ← Clean build system
├── README.md        ← This file
└── archive/         ← Original single-file version (v1)
```

### Data Flow

```
main.cpp
  └─ XcXApp (app)
       ├─ Editor      ← document buffer, cursor, editing
       ├─ FileTree    ← directory tree for sidebar
       ├─ SmartBar    ← modal input overlay
       └─ ThemeManager ← color theme registry
```

### Singleton `common.h` Types

- `ColorPair` — 32 color pair IDs for every UI element
- `ColorDef` — `{fg, bg}` definition
- `Theme` — named color array
- `Editor::Snapshot` — undo/redo state capture

---

## Building

### Requirements
- C++17 compiler (g++ 8+, clang 7+)
- ncurses development headers
- `libstdc++fs` or C++17 filesystem support

### Commands

```bash
make          # Build optimized binary
make debug    # Build with debug symbols (-g -O0)
make clean    # Remove build artifacts
make run      # Build and run
make install  # Copy binary to ~/bin/
make files    # List source files with line counts
```

---

## Configuration

All defaults are hardcoded. Key constants in `common.h`:

```cpp
constexpr size_t UNDO_LIMIT      = 500;   // Undo stack depth
constexpr int    SIDEBAR_WIDTH   = 26;    // Explorer width in cols
constexpr int    LNUM_WIDTH      = 5;     // Line number gutter width
constexpr int    MAX_RESULTS     = 50;    // SmartBar result limit
constexpr int    SCROLL_MARGIN   = 3;     // Cursor scroll margin
```

Themes are defined in `theme.cpp`. To add a new theme:
1. Add a `RawTheme` entry to the `raw_themes[]` array
2. It will be automatically available via `Ctrl+T`

---

## Known Limitations

| Issue | Status |
|---|---|
| No word wrap | Horizontal scroll only |
| No regex search | Plain string matching only |
| No multi-file tabs | Single buffer at a time |
| No LSP / language server | Syntax detection by extension only |
| No mouse support | Keyboard-only navigation |
| No save-as dialog | Save overwrites current file |
| No persistent undo | Per-session only |
| Terminal panel removed | Use a real terminal alongside |

---

## Contributing

XcX is built for people who love terminals. The codebase is clean, modular, and well-commented.

### Good first contributions
- Add a new color theme to `theme.cpp`
- Add syntax highlighting support for a new language
- Improve the fuzzy file matching in `smartbar.cpp`
- Add line-wrapping mode
- Implement persistent configuration via environment variables

### Guidelines
- Keep dependencies at zero beyond ncurses and C++17
- Prefer readability over cleverness
- Test with `make debug` before submitting

---

## License

MIT. Do whatever you want. Attribution appreciated but not required.

---

## Credits

XcX was originally born from a Python CLI editor concept, then rewritten in C++17 by [p1radev](https://github.com/p1rater). v2.0 is a ground-up refactor with themes, proper cursor, modular architecture, and performance improvements.

---

*Made for the terminal. Made for speed. Made for you.*
