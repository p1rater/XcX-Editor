#pragma once

#include "common.h"

class Editor {
public:
    // ---- Document buffer ----
    std::vector<std::string> lines;
    int cy = 0, cx = 0;       // cursor (row, col) — 0-indexed
    int sy = 0, sx = 0;       // scroll offset
    std::string filepath;
    bool modified = false;
    std::string lang = "text";
    int indent_size = 4;
    char indent_char = ' ';

    // ---- Selection ----
    bool mark_active = false;
    int mark_cy = 0, mark_cx = 0;
    std::string clipboard;

    // ---- Undo / Redo ----
    struct Snapshot { std::vector<std::string> lines; int cy; int cx; };
    std::deque<Snapshot> undo_stack;
    std::deque<Snapshot> redo_stack;

    // ---- Hex mode ----
    bool hex_mode = false;

    // ---- Search highlights ----
    std::vector<std::pair<int,int>> search_matches; // {line, col} positions
    int search_match_idx = 0;

    // ---- Cursor blink state ----
    bool cursor_visible = true; // toggled by blink timer

    Editor();

    // --- Buffer operations ---
    void snapshot();
    void undo();
    void redo();
    void load(const std::string& path);
    bool save();
    void new_buffer();

    // --- Editing ---
    void insert_char(char ch);
    void insert_text(const std::string& text);
    void backspace();
    void delete_fwd();
    void newline();
    void tab();
    void untab();

    // --- Cursor movement ---
    void move(int dy, int dx);
    void move_up(int n = 1);
    void move_down(int n = 1);
    void move_left(int n = 1);
    void move_right(int n = 1);
    void home();
    void end();
    void page(int rows, int dir);
    void word_left();
    void word_right();
    void goto_bol();
    void goto_eol();
    void goto_file_begin();
    void goto_file_end();

    // --- Selection ---
    void toggle_mark();
    bool has_selection() const { return mark_active && (mark_cy != cy || mark_cx != cx); }
    std::vector<int> get_selection() const; // {sy, sx, ey, ex}
    std::string get_selected_text() const;
    bool cut_selection();
    void cut_line();
    void copy_selection();
    void copy_line();
    void paste();
    void select_all();
    void delete_selection();

    // --- Navigation ---
    bool go_to_line(int target);
    int line_count() const { return static_cast<int>(lines.size()); }
    int cursor_line() const { return cy + 1; }
    int cursor_col() const { return cx + 1; }

    // --- Bracket matching ---
    std::pair<int,int> find_bracket_match() const;
    bool cursor_on_bracket() const;

    // --- Hex mode ---
    std::string get_hex_line(int line_idx) const;
    void toggle_hex_mode();

    // --- Search ---
    void clear_search_highlights();
    void highlight_matches(const std::string& query);

    // --- Indent ---
    void detect_indent();

    // --- Language ---
    static std::string detect_lang(const std::string& path);

    // --- Scroll management ---
    void scroll_to_cursor(int editor_height, int editor_width);
    void clamp_cursor();
    void ensure_cursor_visible();

private:
    static std::string get_lang_from_ext(const std::string& path);
};
