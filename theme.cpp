#include "theme.h"

// ----------------------------------------------------------------------
// Helper: safe ncurses init_pair wrapper
// ----------------------------------------------------------------------
void init_color_pairs(const Theme& theme) {
    if (!has_colors()) return;
    for (int i = 0; i < NUM_COLOR_PAIRS; ++i) {
        int idx = i + 1; // pairs are 1-indexed in ncurses
        int fg = theme.colors[i].fg;
        int bg = theme.colors[i].bg;
        // -1 means use default (terminal default color)
        if (fg == -1) fg = COLOR_WHITE;
        if (bg == -1) bg = COLOR_BLACK;
        init_pair(idx, fg, bg);
    }
}

// ----------------------------------------------------------------------
// Helper to define a color scheme entry
// ----------------------------------------------------------------------
#define DEF(fg, bg) ColorDef{COLOR_##fg, COLOR_##bg}
#define DEF_DEF    ColorDef{-1, -1}
#define DEF_BLACK  ColorDef{-1, COLOR_BLACK}
#define DEF_WHITE  ColorDef{COLOR_WHITE, -1}

// ----------------------------------------------------------------------
// Theme definitions
// ----------------------------------------------------------------------

struct RawTheme {
    const char* name;
    const char* desc;
    ColorDef colors[NUM_COLOR_PAIRS];
};

// Helper list for all themes
static const RawTheme raw_themes[] = {

    // ================================================================
    // 1. BLUE  — Classic professional blue theme (default)
    // ================================================================
    {
        "Blue", "Classic professional blue theme",
        {
            DEF(WHITE, BLACK),          // CP_DEFAULT
            DEF(WHITE, BLUE),           // CP_TITLE bg=blue
            DEF(WHITE, BLUE),           // CP_TITLE_TEXT
            DEF(WHITE, BLACK),          // CP_SIDEBAR
            DEF(WHITE, BLUE),           // CP_SIDEBAR_SEL
            DEF(BLACK, WHITE),          // CP_GUTTER
            DEF(GREEN, BLACK),          // CP_GUTTER_ADD
            DEF(YELLOW, BLACK),         // CP_GUTTER_MOD
            DEF(RED, BLACK),            // CP_GUTTER_DEL
            DEF(WHITE, BLACK),          // CP_LNUM
            DEF(WHITE, BLUE),           // CP_LNUM_CUR
            DEF(WHITE, BLACK),          // CP_EDITOR
            DEF(BLACK, WHITE),          // CP_EDITOR_CUR
            DEF(WHITE, CYAN),           // CP_CURSOR — bright block cursor
            DEF(CYAN, BLUE),            // CP_SELECTION
            DEF(WHITE, BLUE),           // CP_STATUS
            DEF(YELLOW, BLUE),          // CP_STATUS_MSG
            DEF(CYAN, BLUE),            // CP_STATUS_RIGHT
            DEF(WHITE, BLACK),          // CP_BAR
            DEF(WHITE, BLUE),           // CP_BAR_SEL
            DEF(WHITE, BLACK),          // CP_BAR_INPUT
            DEF(YELLOW, BLACK),         // CP_HEX_HIGHLIGHT
            DEF(YELLOW, MAGENTA),       // CP_BRACKET_MATCH
            DEF(GREEN, BLACK),          // CP_GIT_ADD
            DEF(YELLOW, BLACK),         // CP_GIT_MOD
            DEF(RED, BLACK),            // CP_GIT_DEL
            DEF(WHITE, BLACK),          // CP_STICKY
            DEF(WHITE, BLACK),          // CP_TERMINAL
            DEF(WHITE, BLUE),           // CP_TERMINAL_PROMPT
            DEF(BLUE, BLACK),           // CP_LINE_HIGHLIGHT
            DEF(GREEN, BLACK),          // CP_MATCH_HIGHLIGHT
            DEF(WHITE, BLACK),          // CP_WHITESPACE
            DEF(WHITE, BLACK),          // CP_EOF_TILDE
        }
    },

    // ================================================================
    // 2. RED  — Warm ruby/crimson tones
    // ================================================================
    {
        "Red", "Warm ruby/crimson tones",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(WHITE, RED),
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(BLACK, RED),
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(WHITE, RED),
            DEF(WHITE, RED),
            DEF(WHITE, RED),
            DEF(YELLOW, RED),
            DEF(YELLOW, RED),
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(WHITE, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, RED),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(MAGENTA, BLACK),
            DEF(GREEN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 3. GREEN  — Matrix/forest green theme
    // ================================================================
    {
        "Green", "Matrix-inspired forest green",
        {
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(GREEN, BLACK),
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(GREEN, GREEN),
            DEF(BLACK, GREEN),
            DEF(GREEN, GREEN),
            DEF(YELLOW, GREEN),
            DEF(CYAN, GREEN),
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, GREEN),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(GREEN, BLACK),
            DEF(GREEN, BLACK),
            DEF(BLACK, GREEN),
            DEF(GREEN, BLACK),
            DEF(CYAN, BLACK),
            DEF(GREEN, BLACK),
            DEF(GREEN, BLACK),
        }
    },

    // ================================================================
    // 4. GRAY  — Minimal monochrome grayscale
    // ================================================================
    {
        "Gray", "Minimal monochrome grayscale",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, WHITE),
            DEF(WHITE, BLACK),
            DEF(WHITE, WHITE),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, WHITE),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 5. RGB  — Vibrant rainbow accents
    // ================================================================
    {
        "RGB", "Vibrant rainbow accents on dark",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, BLUE),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(YELLOW, MAGENTA),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, CYAN),
            DEF(CYAN, MAGENTA),
            DEF(WHITE, BLUE),
            DEF(CYAN, BLUE),
            DEF(MAGENTA, BLUE),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(WHITE, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, MAGENTA),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(CYAN, BLACK),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 6. MOON  — Soft, low-contrast dark theme
    // ================================================================
    {
        "Moon", "Soft low-contrast night theme",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, BLUE),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(CYAN, BLACK),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, BLUE),
            DEF(CYAN, BLACK),
            DEF(BLUE, BLACK),
            DEF(YELLOW, BLUE),
            DEF(CYAN, BLUE),
            DEF(WHITE, BLACK),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, BLUE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLUE),
            DEF(CYAN, BLACK),
            DEF(GREEN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 7. OCEAN  — Teal/cyan ocean depths
    // ================================================================
    {
        "Ocean", "Deep teal/cyan ocean tones",
        {
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(CYAN, BLACK),
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(WHITE, CYAN),
            DEF(BLACK, CYAN),
            DEF(CYAN, CYAN),
            DEF(YELLOW, CYAN),
            DEF(WHITE, CYAN),
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(CYAN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, CYAN),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(CYAN, BLACK),
            DEF(CYAN, BLACK),
            DEF(BLACK, CYAN),
            DEF(CYAN, BLACK),
            DEF(GREEN, BLACK),
            DEF(CYAN, BLACK),
            DEF(CYAN, BLACK),
        }
    },

    // ================================================================
    // 8. SUNSET  — Warm orange/purple dusk
    // ================================================================
    {
        "Sunset", "Warm orange/purple dusk tones",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, RED),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(YELLOW, BLACK),
            DEF(BLACK, MAGENTA),
            DEF(YELLOW, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, MAGENTA),
            DEF(BLACK, MAGENTA),
            DEF(MAGENTA, RED),
            DEF(YELLOW, RED),
            DEF(YELLOW, RED),
            DEF(YELLOW, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, MAGENTA),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(MAGENTA, BLACK),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 9. NORD  — Famous arctic, bluish nord palette
    // ================================================================
    {
        "Nord", "Arctic bluish nord palette",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, BLUE),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(CYAN, BLACK),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(CYAN, BLACK),
            DEF(WHITE, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, CYAN),
            DEF(CYAN, BLACK),
            DEF(CYAN, BLACK),
            DEF(YELLOW, CYAN),
            DEF(CYAN, CYAN),
            DEF(WHITE, BLACK),
            DEF(WHITE, CYAN),
            DEF(WHITE, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, CYAN),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, CYAN),
            DEF(CYAN, BLACK),
            DEF(GREEN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },

    // ================================================================
    // 10. DRACULA  — Famous dark purple Dracula theme
    // ================================================================
    {
        "Dracula", "Dark purple Dracula palette",
        {
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(MAGENTA, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(BLACK, WHITE),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(MAGENTA, BLACK),
            DEF(BLACK, WHITE),
            DEF(WHITE, MAGENTA),
            DEF(CYAN, BLACK),
            DEF(MAGENTA, BLACK),
            DEF(YELLOW, MAGENTA),
            DEF(GREEN, MAGENTA),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(WHITE, BLACK),
            DEF(YELLOW, BLACK),
            DEF(YELLOW, MAGENTA),
            DEF(GREEN, BLACK),
            DEF(YELLOW, BLACK),
            DEF(RED, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, MAGENTA),
            DEF(MAGENTA, BLACK),
            DEF(GREEN, BLACK),
            DEF(WHITE, BLACK),
            DEF(WHITE, BLACK),
        }
    },
};

static constexpr int NUM_THEMES = sizeof(raw_themes) / sizeof(raw_themes[0]);

// ----------------------------------------------------------------------
// ThemeManager implementation
// ----------------------------------------------------------------------
ThemeManager::ThemeManager() {
    build_themes();
}

void ThemeManager::build_themes() {
    themes_.resize(NUM_THEMES);
    for (int i = 0; i < NUM_THEMES; ++i) {
        themes_[i].name = raw_themes[i].name;
        themes_[i].description = raw_themes[i].desc;
        std::copy(std::begin(raw_themes[i].colors),
                  std::end(raw_themes[i].colors),
                  themes_[i].colors);
    }
}

void ThemeManager::apply_theme(int index) {
    if (index < 0) index = 0;
    if (index >= count()) index = count() - 1;
    current_ = index;
    init_color_pairs(themes_[current_]);
}

void ThemeManager::next_theme() {
    apply_theme((current_ + 1) % count());
}

void ThemeManager::prev_theme() {
    apply_theme((current_ - 1 + count()) % count());
}

std::vector<std::string> ThemeManager::theme_names() const {
    std::vector<std::string> names;
    names.reserve(themes_.size());
    for (const auto& t : themes_) {
        names.push_back(t.name);
    }
    return names;
}
