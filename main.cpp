// ======================================================================
// XcX Editor v2.0
// A terminal-based text and hex editor with nano-style keybindings,
// color themes, and a beautiful block cursor.
//
// Compile: g++ -std=c++17 -o xcx main.cpp editor.cpp filetree.cpp smartbar.cpp app.cpp theme.cpp -lncurses -lstdc++fs
// ======================================================================

#include "common.h"
#include "app.h"
#include <cstring>
#include <clocale>

int main(int argc, char** argv) {
    // Locale for UTF-8 support (needed for box-drawing chars)
    setlocale(LC_ALL, "");

    // Initialize ncurses
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // hide hardware cursor — we draw our own beautiful block cursor
    set_escdelay(25); // reduce ESC delay

    if (has_colors())
        start_color();

    // Determine root directory and file from arguments
    std::string root = ".";
    std::string file;
    if (argc > 1) {
        fs::path p(argv[1]);
        if (fs::exists(p)) {
            if (fs::is_directory(p)) {
                root = p.string();
            } else {
                file = p.string();
                root = p.parent_path().string();
                if (root.empty()) root = ".";
            }
        }
    }

    // Create and run the application
    XcXApp app(root);
    app.run(file);

    // Cleanup
    endwin();
    return 0;
}
