#pragma once

#include "common.h"
#include "theme.h"
#include "editor.h"
#include "filetree.h"
#include "smartbar.h"
#include <string>
#include <memory>

class XcXApp {
public:
    Editor ed;
    FileTree tree;
    SmartBar bar;
    ThemeManager theme_mgr;
    bool running = true;
    bool zen = false;
    bool help_screen = false;
    bool focus_explorer = false;
    bool show_nano_bar = true;
    std::string msg;
    bool msg_err = false;

    int rows, cols;

    // Cursor blink state
    bool cursor_visible = true;
    int blink_counter = 0;

    XcXApp(const std::string& root);
    ~XcXApp() = default;

    void run(const std::string& initial_file = "");

private:
    // Layout calculations
    int editor_height() const;
    int editor_width() const;
    int editor_start() const;
    int ln_col() const;

    // Rendering
    void render();
    void render_title_bar();
    void render_sidebar();
    void render_editor();
    void render_status_bar();
    void render_bar();
    void draw_box(int y, int x, int h, int w);
    void draw_cursor(int sr, int sc);
    void draw_bracket_match(int br, int bc);
    void draw_selection_highlight(int li, int row, int es);
    void draw_sticky_scroll();

    // Input handling
    void handle_key(int key);
    void handle_editor_key(int key);
    void handle_explorer_key(int key);
    void handle_bar_key(int key);
    void handle_global_key(int key);
    void bar_confirm();

    // Help screen
    void render_help(int es, int ew, int eh);

    // Startup
    void on_open_file(const std::string& file);

    // Cursor blink
    bool check_blink_frame();
};
