#include "app.h"
#include <sys/ioctl.h>
#include <thread>
#include <chrono>

// ======================================================================
// XcXApp — Main Application
// ======================================================================

XcXApp::XcXApp(const std::string& root)
    : tree(root)
{
    getmaxyx(stdscr, rows, cols);
    // Apply default theme
    theme_mgr.apply_theme(0);
    msg = "XcX v2.0  |  F1 Help  Ctrl+T Theme  F11 Zen";
}

// ======================================================================
// Layout calculations
// ======================================================================
int XcXApp::editor_height() const {
    int extra = (zen || !show_nano_bar) ? 2 : 3;
    return std::max(4, rows - extra);
}

int XcXApp::editor_width() const {
    return std::max(10, cols - (zen ? 0 : SIDEBAR_WIDTH + GUTTER_WIDTH + 2) - LNUM_WIDTH - 2);
}

int XcXApp::editor_start() const {
    int col = zen ? 1 : (SIDEBAR_WIDTH + GUTTER_WIDTH + 2);
    return col + LNUM_WIDTH + 1;
}

int XcXApp::ln_col() const {
    return zen ? 1 : (SIDEBAR_WIDTH + GUTTER_WIDTH + 2);
}

// ======================================================================
// Cursor blink
// ======================================================================
bool XcXApp::check_blink_frame() {
    blink_counter = (blink_counter + 1) % 10;
    if (blink_counter == 0) {
        cursor_visible = !cursor_visible;
        return true;
    }
    return false;
}

// ======================================================================
// RUN — Main loop
// ======================================================================
void XcXApp::run(const std::string& initial_file) {
    if (!initial_file.empty()) {
        on_open_file(initial_file);
    }

    // Ensure we respond to window resize
    auto last_check = std::chrono::steady_clock::now();

    nodelay(stdscr, TRUE);  // non-blocking getch

    while (running) {
        // Check for resize every cycle
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_check).count() >= 200) {
            int nr, nc;
            getmaxyx(stdscr, nr, nc);
            if (nr != rows || nc != cols) {
                rows = nr;
                cols = nc;
                resize_term(rows, cols);
                clear();
            }
            last_check = now;
        }

        render();

        // nodelay is set, so getch returns ERR immediately if no key
        int ch = getch();
        if (ch != ERR) {
            msg_err = false;
            msg = "";
            handle_key(ch);
            cursor_visible = true;  // show cursor on keypress
            blink_counter = 0;
        } else {
            check_blink_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
        }
    }
}

// ======================================================================
// Startup
// ======================================================================
void XcXApp::on_open_file(const std::string& file) {
    ed.load(file);
    focus_explorer = false;
    msg = "opened " + fs::path(file).filename().string();
}

// ======================================================================
// RENDER — Main screen render
// ======================================================================
void XcXApp::render() {
    // Full clear every frame to prevent old-content bleed
    clear();

    // Keep cursor in view before drawing
    ed.scroll_to_cursor(editor_height(), editor_width());

    render_title_bar();

    if (!zen) {
        render_sidebar();
    }

    render_editor();

    if (!zen) {
        render_status_bar();
    }

    if (bar.active()) {
        render_bar();
    }

    refresh();
}

// ======================================================================
// Title bar
// ======================================================================
void XcXApp::render_title_bar() {
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    std::string title = " === XcX Editor ===";
    std::string fname;
    if (!ed.filepath.empty()) {
        fname = fs::path(ed.filepath).filename().string();
        if (ed.modified) fname += "*";
    } else {
        fname = "[New File]";
    }
    title += "  " + fname;
    if (zen) title += "  [zen]";
    if (ed.hex_mode) title += "  [HEX]";
    title += "  |  " + theme_mgr.current_name();

    // Truncate safely to terminal width using only ASCII chars
    if ((int)title.size() > cols - 1) title = title.substr(0, cols - 1);
    mvprintw(0, 0, "%s", title.c_str());
    // Clear remainder of line
    for (int c = (int)title.size(); c < cols; ++c) mvprintw(0, c, " ");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
}

// ======================================================================
// Sidebar (file tree)
// ======================================================================
void XcXApp::render_sidebar() {
    int EH = editor_height();

    // Sidebar header
    bool sel_focus = focus_explorer;
    attron(COLOR_PAIR(sel_focus ? CP_SIDEBAR_SEL : CP_SIDEBAR) | A_BOLD);
    mvprintw(1, 0, "%-*s", SIDEBAR_WIDTH, " EXPLORER");
    attroff(COLOR_PAIR(sel_focus ? CP_SIDEBAR_SEL : CP_SIDEBAR) | A_BOLD);

    // Tree items
    for (int i = 0; i < EH - 1; ++i) {
        int idx = tree.scroll + i;
        if (idx >= (int)tree.items.size()) {
            mvprintw(i + 2, 0, "%-*s", SIDEBAR_WIDTH, "");
            continue;
        }
        auto& item = tree.items[idx];
        bool is_current = (idx == tree.selected);

        // Build label
        std::string label;
        if (item.is_dir) {
            label = std::string(item.depth * 2, ' ') +
                    (item.expanded ? "v " : "> ");
        } else {
            label = std::string(item.depth * 2, ' ') + "  ";
        }
        label += item.path.filename().string();

        // Truncate
        if ((int)label.size() > SIDEBAR_WIDTH - 1)
            label = label.substr(0, SIDEBAR_WIDTH - 1);

        int color_pair = (sel_focus && is_current) ? CP_SIDEBAR_SEL : CP_SIDEBAR;
        attron(COLOR_PAIR(color_pair));
        if (sel_focus && is_current) attron(A_BOLD);
        mvprintw(i + 2, 0, "%-*s", SIDEBAR_WIDTH, label.c_str());
        if (sel_focus && is_current) attroff(A_BOLD);
        attroff(COLOR_PAIR(color_pair));
    }

    // Git gutter
    for (int i = 0; i < EH; ++i) {
        mvprintw(i + 1, SIDEBAR_WIDTH, " ");
    }

    // Divider
    int div_col = SIDEBAR_WIDTH + GUTTER_WIDTH + 1;
    for (int r = 1; r < rows - 1; ++r) {
        mvprintw(r, div_col, "|");
    }
}

// ======================================================================
// Editor area
// ======================================================================
void XcXApp::render_editor() {
    int EH = editor_height();
    int EW = editor_width();
    int ES = editor_start();
    int LC  = ln_col();

    // Line numbers
    for (int i = 0; i < EH; ++i) {
        int ln = ed.sy + i + 1;
        if (ln <= (int)ed.lines.size()) {
            bool cur = (ed.sy + i == ed.cy);
            attron(COLOR_PAIR(cur ? CP_LNUM_CUR : CP_LNUM));
            if (cur) attron(A_BOLD);
            mvprintw(i + 1, LC, "%*d ", LNUM_WIDTH - 1, ln);
            if (cur) attroff(A_BOLD);
            attroff(COLOR_PAIR(cur ? CP_LNUM_CUR : CP_LNUM));
        } else {
            // Tilde for empty lines (end of file marker)
            attron(COLOR_PAIR(CP_EOF_TILDE) | A_DIM);
            mvprintw(i + 1, LC, "~%-*s", LNUM_WIDTH - 1, "");
            attroff(COLOR_PAIR(CP_EOF_TILDE) | A_DIM);
        }
    }

    // Help screen
    if (help_screen) {
        render_help(ES, EW, EH);
        return;
    }

    // Draw editor content
    for (int i = 0; i < EH; ++i) {
        int li = ed.sy + i;
        if (li >= (int)ed.lines.size()) {
            mvprintw(i + 1, ES, "%-*s", EW, "");
            continue;
        }
        const std::string& line = ed.lines[li];

        // Selection highlighting
        auto sel = ed.get_selection();
        if (!sel.empty() && li >= sel[0] && li <= sel[2]) {
            draw_selection_highlight(li, i, ES);
            continue;
        }

        // Hex mode display
        if (ed.hex_mode) {
            std::string hex = ed.get_hex_line(li);
            // ASCII side
            std::string ascii;
            for (char c : line)
                ascii += (std::isprint(static_cast<unsigned char>(c)) ? c : '.');
            std::string combined = hex + "  " + ascii;
            if ((int)combined.size() > EW) combined = combined.substr(0, EW);
            mvprintw(i + 1, ES, "%-*s", EW, combined.c_str());

            // Hex cursor
            if (li == ed.cy && cursor_visible) {
                draw_cursor(i + 1, ES + (ed.cx * 3));
            }
            continue;
        }

        // Plain text line
        attron(COLOR_PAIR(CP_EDITOR));
        if ((int)line.size() > EW) {
            mvprintw(i + 1, ES, "%-*s", EW, line.substr(ed.sx, EW).c_str());
        } else {
            mvprintw(i + 1, ES, "%-s", line.c_str());
            // Clear remaining space
            int extra = EW - (int)line.size();
            if (extra > 0) mvprintw(i + 1, ES + (int)line.size(), "%*s", extra, "");
        }
        attroff(COLOR_PAIR(CP_EDITOR));

        // Current line highlight
        if (li == ed.cy && cursor_visible) {
            draw_cursor(i + 1, ES + (ed.cx - ed.sx));
        }
    }

    // Bracket match
    auto match = ed.find_bracket_match();
    if (match.first != -1) {
        int br = match.first - ed.sy + 1;
        int bc = match.second - ed.sx + ES;
        if (br >= 1 && br <= EH && bc >= ES && bc < ES + EW) {
            attron(COLOR_PAIR(CP_BRACKET_MATCH));
            mvprintw(br, bc, "%c", ed.lines[match.first][match.second]);
            attroff(COLOR_PAIR(CP_BRACKET_MATCH));
        }
    }

    // Sticky scroll
    draw_sticky_scroll();
}

// ======================================================================
// Cursor drawing
// ======================================================================
void XcXApp::draw_cursor(int sr, int sc) {
    if (!cursor_visible) return; // blink off
    int EH = editor_height();
    int EW = editor_width();
    int ES = editor_start();

    if (sr < 1 || sr > EH || sc < ES || sc >= ES + EW) return;

    // Draw a bright block cursor over the character
    char ch = ' ';
    if (ed.cy < (int)ed.lines.size() &&
        ed.cx < (int)ed.lines[ed.cy].size())
        ch = ed.lines[ed.cy][ed.cx];

    attron(COLOR_PAIR(CP_CURSOR) | A_REVERSE | A_BOLD);
    mvprintw(sr, sc, "%c", ch);
    attroff(COLOR_PAIR(CP_CURSOR) | A_REVERSE | A_BOLD);
}

// ======================================================================
// Bracket match
// ======================================================================
void XcXApp::draw_bracket_match(int br, int bc) {
    int EH = editor_height();
    int EW = editor_width();
    int ES = editor_start();
    if (br >= 1 && br <= EH && bc >= ES && bc < ES + EW) {
        attron(COLOR_PAIR(CP_BRACKET_MATCH));
        mvprintw(br, bc, "%c", ed.lines[br - 1 + ed.sy][bc - ES + ed.sx]);
        attroff(COLOR_PAIR(CP_BRACKET_MATCH));
    }
}

// ======================================================================
// Selection highlighting
// ======================================================================
void XcXApp::draw_selection_highlight(int li, int row, int es) {
    auto sel = ed.get_selection();
    if (sel.empty()) return;
    int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
    int EW = editor_width();
    const std::string& line = ed.lines[li];
    int line_len = (int)line.size();

    // Determine selection range on this line
    int sel_start = (li == sy) ? sx : 0;
    int sel_end   = (li == ey) ? ex : line_len;

    // Draw non-selected part before selection
    int left_len = std::min(sel_start - ed.sx, EW);
    if (left_len > 0) {
        attron(COLOR_PAIR(CP_EDITOR));
        mvprintw(row + 1, es, "%-*s", left_len,
                 line.substr(ed.sx, left_len).c_str());
        attroff(COLOR_PAIR(CP_EDITOR));
    }

    // Draw selected part
    int sel_len = std::min(sel_end - std::max(sel_start, ed.sx), EW - left_len);
    if (sel_len > 0 && sel_start >= ed.sx) {
        attron(COLOR_PAIR(CP_SELECTION) | A_REVERSE);
        mvprintw(row + 1, es + left_len, "%-*s", sel_len,
                 line.substr(sel_start, sel_len).c_str());
        attroff(COLOR_PAIR(CP_SELECTION) | A_REVERSE);
    }

    // Draw remaining
    int right_start = sel_start + sel_len;
    int right_len = std::min(line_len - right_start, EW - left_len - sel_len);
    if (right_len > 0) {
        attron(COLOR_PAIR(CP_EDITOR));
        mvprintw(row + 1, es + left_len + sel_len, "%-*s", right_len,
                 line.substr(right_start, right_len).c_str());
        attroff(COLOR_PAIR(CP_EDITOR));
    }

    // Clear rest of line
    int drawn = left_len + sel_len + right_len;
    int remaining = EW - drawn;
    if (remaining > 0) {
        mvprintw(row + 1, es + drawn, "%*s", remaining, "");
    }
}

// ======================================================================
// Sticky scroll
// ======================================================================
void XcXApp::draw_sticky_scroll() {
    if (zen || ed.sy <= 0) return;
    int EW = editor_width();
    int ES = editor_start();

    int sticky = -1;
    for (int i = ed.sy - 1; i >= 0; --i) {
        auto& l = ed.lines[i];
        std::string s = l;
        s.erase(0, s.find_first_not_of(" \t"));
        if (s.rfind("def ", 0) == 0 || s.rfind("class ", 0) == 0 ||
            s.rfind("function ", 0) == 0 || (s.rfind("if ", 0) == 0 && s.find(':') != std::string::npos)) {
            sticky = i;
            break;
        }
    }

    if (sticky != -1) {
        std::string stub = ed.lines[sticky].substr(0, EW);
        attron(COLOR_PAIR(CP_STICKY) | A_DIM);
        mvprintw(1, ES, "%-*s", EW, stub.c_str());
        attroff(COLOR_PAIR(CP_STICKY) | A_DIM);
    }
}

// ======================================================================
// Help screen
// ======================================================================
void XcXApp::render_help(int es, int ew, int eh) {
    static const std::vector<std::string> help_lines = {
        "+------------------------------------------------------+",
        "|  XcX Editor v2.0  --  Keyboard Reference              |",
        "+------------------------------------------------------+",
        "|  CTRL+S  Save              CTRL+Q  Quit               |",
        "|  CTRL+Z  Undo              CTRL+Y  Redo               |",
        "|  CTRL+X  Cut               CTRL+C  Copy               |",
        "|  CTRL+V  Paste             CTRL+A  Select All         |",
        "|  CTRL+K  Cut line          CTRL+U  Paste (alt)        |",
        "|  CTRL+F  Grep project      CTRL+P  Quick Open         |",
        "|  CTRL+O  Outline           CTRL+W  Search in file     |",
        "|  CTRL+G  Help              CTRL+R  Replace            |",
        "|  CTRL+T  Next theme        ALT+6   Copy selection     |",
        "|  CTRL+L  Go to line        CTRL+H  Toggle hex mode    |",
        "|  F5      Explorer          F6      Focus editor       |",
        "|  F11     Zen mode          ESC     Cancel             |",
        "|  Home    Smart home        End     Line end           |",
        "|  PgUp    Page up           PgDn    Page down          |",
        "+------------------------------------------------------+",
        "",
        "Press any key to close this help screen.",
    };
    int start_y = (eh - (int)help_lines.size()) / 2 + 1;
    if (start_y < 1) start_y = 1;
    for (int i = 0; i < std::min(eh, (int)help_lines.size()); ++i) {
        int y = start_y + i;
        if (y >= rows - 2) break;
        attron(COLOR_PAIR(CP_DEFAULT));
        mvprintw(y, es, "%-*s", ew, help_lines[i].c_str());
        attroff(COLOR_PAIR(CP_DEFAULT));
    }
}

// ======================================================================
// Status bar
// ======================================================================
void XcXApp::render_status_bar() {
    // Nano-style shortcut bar
    if (show_nano_bar) {
        attron(COLOR_PAIR(CP_DEFAULT));
        if (focus_explorer) {
            mvprintw(rows - 2, 0, "%-*s", cols,
                "F1 Help  ^T Theme  up/down Navigate  Enter Open  Backspace Up  Esc Editor");
        } else {
            mvprintw(rows - 2, 0, "%-*s", cols,
                "F1 Help  ^O Save  ^W Search  ^K Cut  ^U Paste  ^A Mark  ^L Goto  ^T Theme  ^Q Quit");
        }
        attroff(COLOR_PAIR(CP_DEFAULT));
    }

    // Status line
    attron(COLOR_PAIR(CP_STATUS));
    std::string mode_str = focus_explorer ? "EXPLORE" : (ed.hex_mode ? "HEX" : "INSERT");
    std::string fname = ed.filepath.empty() ? "[New File]" : fs::path(ed.filepath).filename().string();
    if (ed.modified) fname += " *";

    std::string left = " " + mode_str + "  " + fname;
    std::string right =
        " Ln " + std::to_string(ed.cy + 1) + "," + std::to_string(ed.cx + 1) +
        "  " + std::to_string(ed.line_count()) + "L" +
        "  " + (ed.indent_char == '\t' ? "tab" : std::to_string(ed.indent_size) + "sp") +
        "  " + ed.lang;

    // Theme indicator on right
    right += "  [" + theme_mgr.current_name() + "]";

    int mid = cols - (int)left.size() - (int)right.size() - 2;
    if (mid < 0) mid = 0;
    std::string center = msg;
    if ((int)center.size() > mid) center = center.substr(0, mid);

    attron(COLOR_PAIR(msg_err ? CP_STATUS_MSG : CP_STATUS));
    mvprintw(rows - 1, 0, "%s", left.c_str());
    attron(COLOR_PAIR(CP_STATUS));
    mvprintw(rows - 1, (int)left.size(), "%-*s", mid, center.c_str());
    attron(COLOR_PAIR(CP_STATUS_RIGHT));
    mvprintw(rows - 1, (int)left.size() + mid, "%s", right.c_str());
    attroff(COLOR_PAIR(CP_STATUS_RIGHT) | A_BOLD);
    attroff(COLOR_PAIR(CP_STATUS));
}

// ======================================================================
// SmartBar overlay rendering
// ======================================================================
void XcXApp::render_bar() {
    int w = std::min(cols - 4, 60);
    int x = (cols - w) / 2;
    int h = 8;  // base height
    int y = rows / 2 - h / 2;

    std::string title = bar.mode_title();
    int iw = w - 2; // inner width

    // Draw the box
    attron(COLOR_PAIR(CP_BAR));
    mvprintw(y, x, "+");
    mvprintw(y, x + 1, "-%s", title.c_str());
    int title_end = 1 + (int)title.size();
    for (int j = title_end; j < w - 1; ++j) mvprintw(y, x + j, "-");
    mvprintw(y, x + w - 1, "+");

    if (bar.mode == SmartBar::REPLACE) {
        // Two-field replace mode
        std::string find = "> " + bar.text + (bar.replace_field == 0 ? "_" : " ");
        std::string repl = "| " + bar.replace_text + (bar.replace_field == 1 ? "_" : " ");
        mvprintw(y + 1, x, "| %-*s |", iw - 2, find.c_str());
        mvprintw(y + 2, x, "| %-*s |", iw - 2, repl.c_str());
        // Separator
        mvprintw(y + 3, x, "+");
        for (int j = 1; j < w - 1; ++j) mvprintw(y + 3, x + j, "-");
        mvprintw(y + 3, x + w - 1, "+");
        int row = y + 4;
        for (int i = 0; i < std::min(5, (int)bar.results.size()); ++i) {
            bool sel = (i == bar.idx);
            attron(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
            mvprintw(row + i, x, "| %-*s |", iw - 2, bar.results[i].c_str());
            attroff(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
        }
        h = 4 + std::min(5, (int)bar.results.size()) + 2;
    } else {
        // Single-input modes
        std::string input = "> " + bar.text + "_";
        mvprintw(y + 1, x, "| %-*s |", iw - 2, input.c_str());
        // Separator
        mvprintw(y + 2, x, "+");
        for (int j = 1; j < w - 1; ++j) mvprintw(y + 2, x + j, "-");
        mvprintw(y + 2, x + w - 1, "+");

        int row = y + 3;
        for (int i = 0; i < std::min(5, (int)bar.results.size()); ++i) {
            bool sel = (i == bar.idx);
            attron(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
            mvprintw(row + i, x, "| %-*s |", iw - 2, bar.results[i].c_str());
            attroff(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
        }
        h = 3 + std::min(5, (int)bar.results.size()) + 2;
    }
    attroff(COLOR_PAIR(CP_BAR));

    // Bottom hint
    int hint_y = y + h - 1;
    if (hint_y <= rows - 2) {
        std::string hint;
        switch (bar.mode) {
            case SmartBar::SEARCH:
                hint = "up/down select  Enter jump  Esc cancel";
                break;
            case SmartBar::GOTO:
                hint = "Enter go to line  Esc cancel";
                break;
            case SmartBar::REPLACE:
                hint = "Tab switch fields  Enter replace all  Esc cancel";
                break;
            case SmartBar::CMD:
                hint = "up/down select  Enter execute  Esc cancel";
                break;
            default:
                hint = "Enter confirm  Esc cancel";
                break;
        }
        mvprintw(hint_y, x, "+-- %-*s --+", w - 6, hint.c_str());
    }
}

// ======================================================================
// KEY HANDLING — Main dispatcher
// ======================================================================
void XcXApp::handle_key(int key) {
    if (help_screen) {
        help_screen = false;
        return;
    }

    if (bar.active()) {
        handle_bar_key(key);
        return;
    }

    if (focus_explorer) {
        handle_explorer_key(key);
        return;
    }

    // Check global shortcuts first
    handle_global_key(key);
}

// ======================================================================
// Global shortcuts (work regardless of focus)
// ======================================================================
void XcXApp::handle_global_key(int key) {
    switch (key) {
        // --- Quit ---
        case 17: // Ctrl+Q
            if (ed.modified) {
                msg = "Unsaved changes! ^S to save, ^Q again to quit";
                msg_err = true;
                // Wait for second ^Q
                timeout(2000);
                int ch2 = getch();
                timeout(-1);
                if (ch2 == 17) { running = false; return; }
                msg = "";
                return;
            }
            running = false;
            return;

        // --- Save ---
        case 19: { // Ctrl+S
            if (ed.filepath.empty()) {
                // No filename set — open a path bar
                bar.open(SmartBar::PATH, "./");
                bar.update(ed, tree.root);
                return;
            }
            bool ok = ed.save();
            msg = ok ? ("Saved " + fs::path(ed.filepath).filename().string()) : "Save failed!";
            msg_err = !ok;
            return;
        }

        // --- Help ---
        case 7: // Ctrl+G
            help_screen = true;
            return;

        // --- Theme switching ---
        case 20: { // Ctrl+T
            theme_mgr.next_theme();
            msg = "Theme: " + theme_mgr.current_name();
            return;
        }

        // --- Zen ---
        case KEY_F(11):
            zen = !zen;
            msg = zen ? "Zen mode ON" : "Zen mode OFF";
            return;

        // --- Explorer focus ---
        case KEY_F(5):
            focus_explorer = true;
            if (tree.empty()) tree.refresh();
            msg = "Explorer";
            return;

        // --- Editor focus ---
        case KEY_F(6):
            focus_explorer = false;
            msg = "Editor";
            return;

        // --- Grep (Ctrl+F) ---
        case 6:
            bar.open(SmartBar::GREP);
            bar.update(ed, tree.root);
            return;

        // --- Quick Open (Ctrl+P) ---
        case 16:
            bar.open(SmartBar::OPEN);
            bar.update(ed, tree.root);
            return;

        // --- Outline (Ctrl+O) ---
        case 15:
            bar.open(SmartBar::OUTLINE);
            bar.update(ed, tree.root);
            return;

        // --- Search (Ctrl+W) ---
        case 23:
            bar.open(SmartBar::SEARCH);
            bar.update(ed, tree.root);
            return;

        // --- Replace (Ctrl+R) ---
        case 18:
            bar.open(SmartBar::REPLACE);
            bar.update(ed, tree.root);
            return;

        // --- Go to line (Ctrl+L) ---
        case 12:
            bar.open(SmartBar::GOTO, std::to_string(ed.cy + 1));
            bar.update(ed, tree.root);
            return;

        // --- Command palette (Alt+X / Ctrl+Shift+P) ---
        case 24: // Ctrl+X
            // Ctrl+X: cut selection or line
            if (ed.has_selection()) {
                ed.cut_selection();
                msg = "Cut selection";
            } else {
                ed.cut_line();
                msg = "Cut line";
            }
            return;
        case 3: // Ctrl+C
            if (ed.has_selection()) {
                ed.copy_selection();
                msg = "Copied";
            } else {
                ed.copy_line();
                msg = "Copied line";
            }
            return;
        case 22: // Ctrl+V
            ed.paste();
            msg = "Pasted";
            return;
        case 1: // Ctrl+A
            ed.select_all();
            msg = "Select all";
            return;
        case 21: // Ctrl+U
            ed.paste();
            msg = "Paste";
            return;

        // --- Undo/Redo ---
        case 26: // Ctrl+Z
            ed.undo();
            msg = "Undo";
            return;
        case 25: // Ctrl+Y
            ed.redo();
            msg = "Redo";
            return;

        // --- Hex mode (Ctrl+H) ---
        case 8:
            ed.toggle_hex_mode();
            msg = ed.hex_mode ? "Hex mode ON" : "Text mode";
            return;

        // --- ESC sequences (Alt+ combinations) ---
        case 27: {
            int next = getch();
            if (next == ERR || next == 27) {
                // Double ESC or just ESC
                focus_explorer = false;
                if (ed.mark_active) {
                    ed.mark_active = false;
                    msg = "Mark unset";
                }
                return;
            }
            // Alt+ combinations
            switch (next) {
                case '6': case 'L':
                    if (ed.has_selection()) ed.copy_selection();
                    else ed.copy_line();
                    msg = "Copied";
                    return;
                case 'w': case 'W':
                    bar.open(SmartBar::PATH, tree.root.string());
                    bar.update(ed, tree.root);
                    return;
                case 'x': case 'X':
                    // Command palette via Alt+X
                    bar.open(SmartBar::CMD);
                    bar.update(ed, tree.root);
                    return;
                default:
                    // Let editor handle remaining sequences
                    ungetch(next);
                    handle_editor_key(27);
                    return;
            }
        }

        default:
            handle_editor_key(key);
    }
}

// ======================================================================
// Editor mode key handling
// ======================================================================
void XcXApp::handle_editor_key(int key) {
    switch (key) {
        case KEY_UP:          ed.move_up(); break;
        case KEY_DOWN:        ed.move_down(); break;
        case KEY_LEFT:        ed.move_left(); break;
        case KEY_RIGHT:       ed.move_right(); break;
        case KEY_HOME:        ed.home(); break;
        case KEY_END:         ed.end(); break;
        case KEY_PPAGE:       ed.page(editor_height() - 2, -1); break;
        case KEY_NPAGE:       ed.page(editor_height() - 2, 1); break;
        case '\n': case '\r': ed.newline(); break;
        case KEY_BACKSPACE: case 127: ed.backspace(); break;
        case KEY_DC:          ed.delete_fwd(); break;
        case '\t':            ed.tab(); break;

        // Ctrl+Left / Ctrl+Right word jump (escape sequences)
        case KEY_SLEFT: case 546: // terminfo shift+left
        case KEY_SRIGHT: case 561: // terminfo shift+right
            break; // ignore for now

        // Mouse clicks — ignore
        case KEY_MOUSE:
            break;

        // Printable characters
        default:
            if (key >= 32 && key <= 126) {
                // Delete selection if exists before inserting
                if (ed.has_selection()) {
                    ed.delete_selection();
                }
                ed.insert_char((char)key);
            } else if (key == 27) {
                // ESC sequence for Ctrl+arrows, etc.
                int next1 = getch();
                if (next1 == '[') {
                    int next2 = getch();
                    if (next2 == 'D') { ed.word_left(); }
                    else if (next2 == 'C') { ed.word_right(); }
                    else if (next2 == '5' || next2 == '1') {
                        // Could be ESC [ 5 D / C or ESC [ 1 ; 5 D / C
                        int next3 = getch();
                        if (next3 == ';' || next3 == '~') {
                            if (next2 == '5' && next3 == '~') {
                                ed.page(editor_height() - 2, -1); // PgUp fallback
                            }
                            // flush remaining
                            int extra;
                            while ((extra = getch()) != ERR && extra != 27 && extra != '[') {}
                        } else if (next3 == 'D') {
                            ed.word_left();
                        } else if (next3 == 'C') {
                            ed.word_right();
                        }
                    }
                }
            }
            break;
    }
}

// ======================================================================
// Explorer mode key handling
// ======================================================================
void XcXApp::handle_explorer_key(int key) {
    switch (key) {
        case KEY_UP:
            tree.move(-1);
            break;
        case KEY_DOWN:
            tree.move(1);
            break;
        case KEY_BACKSPACE: case 127:
            tree.go_up();
            msg = "up " + tree.root.string();
            break;
        case '\n': case '\r': {
            auto* item = tree.current();
            if (!item) return;
            if (item->is_dir) {
                tree.toggle();
            } else {
                ed.load(item->path.string());
                focus_explorer = false;
                msg = "Opened " + item->path.filename().string();
            }
            break;
        }
        case 27: // ESC
            focus_explorer = false;
            msg = "Editor";
            break;
        case 20: // Ctrl+T — theme switch
            theme_mgr.next_theme();
            msg = "Theme: " + theme_mgr.current_name();
            break;
        default:
            break;
    }
}

// ======================================================================
// SmartBar key handling
// ======================================================================
void XcXApp::handle_bar_key(int key) {
    switch (key) {
        case 27: // ESC
            bar.close();
            msg = "";
            break;
        case '\n': case '\r':
            bar_confirm();
            break;
        case KEY_UP:
            bar.nav(-1);
            break;
        case KEY_DOWN:
            bar.nav(1);
            break;
        case '\t':
            if (bar.mode == SmartBar::REPLACE)
                bar.replace_field = 1 - bar.replace_field;
            break;
        case KEY_BACKSPACE: case 127:
            bar.backspace();
            bar.update(ed, tree.root);
            break;
        default:
            if (key >= 32 && key <= 126) {
                bar.type((char)key);
                bar.update(ed, tree.root);
            }
            break;
    }
}

// ======================================================================
// SmartBar confirm
// ======================================================================
void XcXApp::bar_confirm() {
    switch (bar.mode) {
        case SmartBar::GOTO: {
            try {
                int ln = std::stoi(bar.text);
                if (ed.go_to_line(ln))
                    msg = "Line " + std::to_string(ln);
                else
                    msg = "Invalid line (1-" + std::to_string(ed.line_count()) + ")";
            } catch (...) { msg = "Invalid line number"; }
            bar.close();
            break;
        }

        case SmartBar::SEARCH:
        case SmartBar::OUTLINE: {
            int ln = bar.hit_line();
            if (ln >= 0) {
                ed.go_to_line(ln + 1);
                msg = "Jumped to line " + std::to_string(ln + 1);
            }
            bar.close();
            break;
        }

        case SmartBar::PATH: {
            fs::path p(bar.text);
            if (fs::is_directory(p)) {
                tree.root = fs::canonical(p);
                tree.selected = tree.scroll = 0;
                tree.refresh();
                focus_explorer = true;
                msg = "-> " + tree.root.string();
            } else if (fs::exists(p)) {
                ed.load(p.string());
                focus_explorer = false;
                msg = "Opened " + p.filename().string();
            } else {
                msg = "Not found: " + p.string();
                msg_err = true;
            }
            bar.close();
            break;
        }

        case SmartBar::OPEN: {
            std::string f = bar.hit_file();
            if (!f.empty() && fs::exists(f)) {
                ed.load(f);
                focus_explorer = false;
                msg = "Opened " + fs::path(f).filename().string();
            } else if (!bar.text.empty()) {
                // Maybe the text is a partial path — try fuzzy
                msg = "File not found";
                msg_err = true;
            }
            bar.close();
            break;
        }

        case SmartBar::GREP: {
            auto hit = bar.hit_grep();
            if (!hit.first.empty()) {
                fs::path full = tree.root / hit.first;
                if (!fs::exists(full))
                    full = hit.first;
                if (fs::exists(full)) {
                    ed.load(full.string());
                    if (hit.second >= 0) {
                        ed.cy = std::min(hit.second, ed.line_count() - 1);
                        ed.cx = 0;
                    }
                    focus_explorer = false;
                    msg = "Opened " + full.filename().string();
                }
            }
            bar.close();
            break;
        }

        case SmartBar::REPLACE: {
            std::string find = bar.text;
            std::string repl = bar.replace_text;
            if (find.empty()) { bar.close(); return; }
            int count = 0;
            ed.snapshot();
            for (auto& line : ed.lines) {
                size_t pos = 0;
                while ((pos = line.find(find, pos)) != std::string::npos) {
                    line.replace(pos, find.size(), repl);
                    pos += repl.size();
                    count++;
                }
            }
            if (count > 0) {
                ed.modified = true;
                msg = "Replaced " + std::to_string(count) + " occurrence(s)";
            } else {
                msg = "No matches found";
            }
            bar.close();
            break;
        }

        case SmartBar::CMD: {
            std::string cmd = bar.hit_cmd();
            bar.close();
            if (cmd == "save") {
                if (ed.save()) msg = "Saved " + fs::path(ed.filepath).filename().string();
                else msg = "Save failed";
            } else if (cmd == "new") {
                ed.new_buffer();
                msg = "New buffer";
            } else if (cmd == "help") {
                help_screen = true;
            } else if (cmd == "zen") {
                zen = !zen;
                msg = zen ? "Zen ON" : "Zen OFF";
            } else if (cmd == "theme next") {
                theme_mgr.next_theme();
                msg = "Theme: " + theme_mgr.current_name();
            } else if (cmd == "theme prev") {
                theme_mgr.prev_theme();
                msg = "Theme: " + theme_mgr.current_name();
            } else if (cmd == "outline") {
                bar.open(SmartBar::OUTLINE);
                bar.update(ed, tree.root);
            } else if (cmd == "hex") {
                ed.toggle_hex_mode();
                msg = ed.hex_mode ? "Hex ON" : "Hex OFF";
            } else if (cmd == "undo") {
                ed.undo();
                msg = "Undo";
            } else if (cmd == "redo") {
                ed.redo();
                msg = "Redo";
            } else if (cmd == "select all") {
                ed.select_all();
                msg = "Select all";
            } else if (cmd == "exit" || cmd == "quit") {
                if (ed.modified) {
                    msg = "Unsaved changes! Save first";
                    msg_err = true;
                } else {
                    running = false;
                }
            } else if (cmd == "git status") {
                // Run git status
                std::string gitcmd = "cd " + tree.root.string() + " && git status --short 2>/dev/null | head -5";
                FILE* pipe = popen(gitcmd.c_str(), "r");
                if (pipe) {
                    std::string result;
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe)) result += buf;
                    pclose(pipe);
                    msg = result.empty() ? "Clean working tree" : result.substr(0, 80);
                } else {
                    msg = "git not available";
                }
            } else if (cmd == "git log") {
                std::string gitcmd = "cd " + tree.root.string() + " && git log --oneline -5 2>/dev/null";
                FILE* pipe = popen(gitcmd.c_str(), "r");
                if (pipe) {
                    std::string result;
                    char buf[256];
                    while (fgets(buf, sizeof(buf), pipe)) result += buf;
                    pclose(pipe);
                    msg = result.empty() ? "No commits" : result.substr(0, 80);
                } else {
                    msg = "git not available";
                }
            } else {
                msg = "Unknown command: " + cmd;
                msg_err = true;
            }
            break;
        }

        default:
            bar.close();
    }
}
