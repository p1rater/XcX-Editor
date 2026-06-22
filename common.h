#pragma once

#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <climits>
#include <memory>

namespace fs = std::filesystem;

constexpr size_t UNDO_LIMIT      = 500;
constexpr int    SIDEBAR_WIDTH   = 26;
constexpr int    GUTTER_WIDTH    = 1;
constexpr int    LNUM_WIDTH      = 5;
constexpr int    TERM_PANEL_H    = 12;
constexpr int    MAX_RESULTS     = 50;
constexpr int    SCROLL_MARGIN   = 3;

enum ColorPair {
    CP_DEFAULT     = 1,
    CP_TITLE,
    CP_TITLE_TEXT,
    CP_SIDEBAR,
    CP_SIDEBAR_SEL,
    CP_GUTTER,
    CP_GUTTER_ADD,
    CP_GUTTER_MOD,
    CP_GUTTER_DEL,
    CP_LNUM,
    CP_LNUM_CUR,
    CP_EDITOR,
    CP_EDITOR_CUR,
    CP_CURSOR,
    CP_SELECTION,
    CP_STATUS,
    CP_STATUS_MSG,
    CP_STATUS_RIGHT,
    CP_BAR,
    CP_BAR_SEL,
    CP_BAR_INPUT,
    CP_HEX_HIGHLIGHT,
    CP_BRACKET_MATCH,
    CP_GIT_ADD,
    CP_GIT_MOD,
    CP_GIT_DEL,
    CP_STICKY,
    CP_TERMINAL,
    CP_TERMINAL_PROMPT,
    CP_LINE_HIGHLIGHT,
    CP_MATCH_HIGHLIGHT,
    CP_WHITESPACE,
    CP_EOF_TILDE,
    NUM_COLOR_PAIRS
};

struct ColorDef {
    int fg;
    int bg;
};

// Forward declarations
class Editor;
class FileTree;
class SmartBar;
class ThemeManager;
class XcXApp;
