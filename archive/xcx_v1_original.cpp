// xcx.cpp – XcX Editor: a terminal-based text and hex editor with nano-style keybindings.
// Compile: g++ -std=c++17 -o xcx xcx.cpp -lncurses -lstdc++fs

#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <climits>

namespace fs = std::filesystem;

// ----------------------------------------------------------------------
// Terminal utilities (colors, attributes)
// ----------------------------------------------------------------------
enum ColorPair {
    CP_DEFAULT = 1,
    CP_TITLE,
    CP_SIDEBAR,
    CP_SIDEBAR_SEL,
    CP_GUTTER,
    CP_LNUM,
    CP_LNUM_CUR,
    CP_EDITOR,
    CP_EDITOR_CUR,
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
};

void init_colors() {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    init_pair(CP_DEFAULT, COLOR_WHITE, -1);
    init_pair(CP_TITLE, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_SIDEBAR, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_SIDEBAR_SEL, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_GUTTER, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_LNUM, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_LNUM_CUR, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_EDITOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_EDITOR_CUR, COLOR_BLACK, COLOR_WHITE);
    init_pair(CP_STATUS, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_STATUS_MSG, COLOR_YELLOW, COLOR_BLUE);
    init_pair(CP_STATUS_RIGHT, COLOR_CYAN, COLOR_BLUE);
    init_pair(CP_BAR, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_BAR_SEL, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_BAR_INPUT, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_HEX_HIGHLIGHT, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_BRACKET_MATCH, COLOR_YELLOW, COLOR_MAGENTA);
    init_pair(CP_GIT_ADD, COLOR_GREEN, COLOR_BLACK);
    init_pair(CP_GIT_MOD, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CP_GIT_DEL, COLOR_RED, COLOR_BLACK);
    init_pair(CP_STICKY, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_TERMINAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(CP_TERMINAL_PROMPT, COLOR_WHITE, COLOR_BLUE);
}

// ----------------------------------------------------------------------
// Syntax highlighting (simplified)
// ----------------------------------------------------------------------
std::string highlight_line(const std::string& line, const std::string& lang) {
    // Very simplified; just returns the line as-is. 
    // In a full implementation we'd add color codes using attron/attroff.
    // For this C++ version, we skip true syntax highlighting to keep the code manageable.
    // We'll just highlight comments and strings rudimentarily if we wanted.
    return line;
}

// ----------------------------------------------------------------------
// Editor State
// ----------------------------------------------------------------------
class Editor {
public:
    std::vector<std::string> lines;
    int cy = 0, cx = 0;
    int sy = 0, sx = 0;
    std::string filepath;
    bool modified = false;
    std::string lang = "text";
    int indent_size = 4;
    char indent_char = ' ';

    // Selection
    bool mark_active = false;
    int mark_cy = 0, mark_cx = 0;
    std::string clipboard;

    // Undo / Redo
    struct Snapshot { std::vector<std::string> lines; int cy; int cx; };
    std::deque<Snapshot> undo_stack, redo_stack;
    static constexpr size_t UNDO_LIMIT = 500;

    // Hex mode
    bool hex_mode = false;

    Editor() { lines.push_back(""); }

    void snapshot() {
        Snapshot s{lines, cy, cx};
        if (!undo_stack.empty() && undo_stack.back().lines == s.lines &&
            undo_stack.back().cy == s.cy && undo_stack.back().cx == s.cx)
            return;
        undo_stack.push_back(s);
        if (undo_stack.size() > UNDO_LIMIT) undo_stack.pop_front();
        redo_stack.clear();
    }

    void undo() {
        if (undo_stack.empty()) return;
        redo_stack.push_back({lines, cy, cx});
        auto s = undo_stack.back(); undo_stack.pop_back();
        lines = s.lines; cy = s.cy; cx = s.cx;
        modified = true;
    }

    void redo() {
        if (redo_stack.empty()) return;
        undo_stack.push_back({lines, cy, cx});
        auto s = redo_stack.back(); redo_stack.pop_back();
        lines = s.lines; cy = s.cy; cx = s.cx;
        modified = true;
    }

    void load(const std::string& path) {
        filepath = path;
        lang = get_lang(path);
        std::ifstream f(path);
        if (!f) { lines = {""}; return; }
        lines.clear();
        std::string line;
        while (std::getline(f, line)) lines.push_back(line);
        if (lines.empty()) lines.push_back("");
        cy = cx = sy = sx = 0;
        modified = false;
        undo_stack.clear(); redo_stack.clear();
        detect_indent();
        mark_active = false;
    }

    bool save() {
        if (filepath.empty()) return false;
        std::ofstream f(filepath);
        if (!f) return false;
        for (size_t i = 0; i < lines.size(); ++i) {
            f << lines[i];
            if (i + 1 < lines.size()) f << '\n';
        }
        modified = false;
        return true;
    }

    void insert_char(char ch) {
        snapshot();
        auto& l = lines[cy];
        // auto-close pairs
        static const std::map<char,char> pairs = {{'(',')'},{'[',']'},{'{','}'},{'"','"'},{'\'','\''},{'`','`'}};
        static const std::set<char> closing = {')',']','}','"','\'','`'};
        if (pairs.count(ch) && !closing.count(ch)) {
            char closing_char = pairs.at(ch);
            l.insert(cx, 1, ch);
            l.insert(cx+1, 1, closing_char);
            cx++;
            modified = true;
            return;
        }
        // skip closing
        if (closing.count(ch) && cx < (int)l.size() && l[cx] == ch) {
            cx++;
            return;
        }
        l.insert(cx, 1, ch);
        cx++;
        modified = true;
    }

    void backspace() {
        snapshot();
        auto& l = lines[cy];
        static const std::map<char,char> pairs = {{'(',')'},{'[',']'},{'{','}'}};
        if (cx > 0 && cx < (int)l.size() && pairs.count(l[cx-1]) && l[cx] == pairs.at(l[cx-1])) {
            l.erase(cx-1, 2);
            cx--;
            modified = true;
            return;
        }
        if (cx > 0) {
            l.erase(cx-1, 1);
            cx--;
            modified = true;
        } else if (cy > 0) {
            cx = lines[cy-1].size();
            lines[cy-1] += lines[cy];
            lines.erase(lines.begin() + cy);
            cy--;
            modified = true;
        }
    }

    void delete_fwd() {
        snapshot();
        auto& l = lines[cy];
        if (cx < (int)l.size()) {
            l.erase(cx, 1);
            modified = true;
        } else if (cy + 1 < (int)lines.size()) {
            lines[cy] += lines[cy+1];
            lines.erase(lines.begin() + cy + 1);
            modified = true;
        }
    }

    void newline() {
        snapshot();
        auto& l = lines[cy];
        int indent = l.find_first_not_of(" \t");
        if (indent == (int)std::string::npos) indent = l.size();
        if (!l.empty() && (l.back() == ':' || l.back() == '{')) indent += indent_size;
        std::string rest = l.substr(cx);
        l.erase(cx);
        std::string spaces(indent, indent_char == '\t' ? '\t' : ' ');
        lines.insert(lines.begin() + cy + 1, spaces + rest);
        cy++;
        cx = indent;
        modified = true;
    }

    void tab() {
        snapshot();
        if (indent_char == '\t') {
            insert_char('\t');
        } else {
            int spaces = indent_size - (cx % indent_size);
            lines[cy].insert(cx, spaces, ' ');
            cx += spaces;
            modified = true;
        }
    }

    void move(int dy, int dx) {
        cy = std::max(0, std::min((int)lines.size()-1, cy + dy));
        if (dx != 0) cx = std::max(0, std::min((int)lines[cy].size(), cx + dx));
        else cx = std::min(cx, (int)lines[cy].size());
    }

    void home() {
        int indent = lines[cy].find_first_not_of(" \t");
        if (indent == (int)std::string::npos) indent = lines[cy].size();
        cx = (cx == indent) ? 0 : indent;
    }

    void end() { cx = lines[cy].size(); }

    void page(int rows, int dir) {
        cy = std::max(0, std::min((int)lines.size()-1, cy + dir * rows));
        cx = std::min(cx, (int)lines[cy].size());
    }

    void word_left() {
        while (cx > 0 && !std::isalnum(lines[cy][cx-1])) --cx;
        while (cx > 0 && std::isalnum(lines[cy][cx-1])) --cx;
    }

    void word_right() {
        auto& l = lines[cy];
        while (cx < (int)l.size() && !std::isalnum(l[cx])) ++cx;
        while (cx < (int)l.size() && std::isalnum(l[cx])) ++cx;
    }

    // Selection
    void toggle_mark() {
        mark_active = !mark_active;
        if (mark_active) { mark_cy = cy; mark_cx = cx; }
    }

    // returns selection rect: {start_y, start_x, end_y, end_x} or empty if inactive
    std::vector<int> get_selection() const {
        if (!mark_active) return {};
        int sy = mark_cy, sx = mark_cx, ey = cy, ex = cx;
        if (sy > ey || (sy == ey && sx > ex)) {
            std::swap(sy, ey); std::swap(sx, ex);
        }
        return {sy, sx, ey, ex};
    }

    std::string get_selected_text() const {
        auto sel = get_selection();
        if (sel.empty()) return "";
        int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
        if (sy == ey) return lines[sy].substr(sx, ex - sx);
        std::string result = lines[sy].substr(sx);
        for (int i = sy+1; i < ey; ++i) result += '\n' + lines[i];
        result += '\n' + lines[ey].substr(0, ex);
        return result;
    }

    bool cut_selection() {
        auto sel = get_selection();
        if (sel.empty()) return false;
        snapshot();
        int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
        clipboard = get_selected_text();
        if (sy == ey) {
            lines[sy].erase(sx, ex - sx);
            cy = sy; cx = sx;
        } else {
            lines[sy].erase(sx);
            lines[sy] += lines[ey].substr(ex);
            for (int i = ey; i > sy; --i) lines.erase(lines.begin() + i);
            cy = sy; cx = sx;
        }
        mark_active = false;
        modified = true;
        return true;
    }

    void cut_line() {
        snapshot();
        clipboard = lines[cy] + '\n';
        if (lines.size() == 1) {
            lines[0].clear();
            cx = 0;
        } else {
            lines.erase(lines.begin() + cy);
            if (cy >= (int)lines.size()) cy = lines.size()-1;
            cx = std::min(cx, (int)lines[cy].size());
        }
        mark_active = false;
        modified = true;
    }

    void copy_selection() {
        auto txt = get_selected_text();
        if (!txt.empty()) clipboard = txt;
    }

    void paste() {
        if (clipboard.empty()) return;
        snapshot();
        std::vector<std::string> paste_lines;
        std::stringstream ss(clipboard);
        std::string line;
        while (std::getline(ss, line)) paste_lines.push_back(line);
        if (paste_lines.empty()) return;
        if (paste_lines.size() == 1) {
            lines[cy].insert(cx, paste_lines[0]);
            cx += paste_lines[0].size();
        } else {
            std::string rest = lines[cy].substr(cx);
            lines[cy].erase(cx);
            lines[cy] += paste_lines[0];
            for (size_t i = 1; i < paste_lines.size(); ++i) {
                lines.insert(lines.begin() + cy + i, paste_lines[i]);
            }
            lines[cy + paste_lines.size() - 1] += rest;
            cy += paste_lines.size() - 1;
            cx = paste_lines.back().size();
        }
        modified = true;
        mark_active = false;
    }

    bool go_to_line(int target) {
        if (target < 1 || target > (int)lines.size()) return false;
        cy = target - 1;
        cx = 0;
        return true;
    }

    // bracket matching
    std::pair<int,int> find_bracket_match() const {
        if (cy >= (int)lines.size()) return {-1,-1};
        const auto& l = lines[cy];
        if (cx >= (int)l.size()) return {-1,-1};
        char ch = l[cx];
        static const std::set<char> open_br = {'(','[','{'};
        static const std::set<char> close_br = {')',']','}'};
        static const std::map<char,char> match = {{')','('},{']','['},{'}','{'},{'(',')'},{'[',']'},{'{','}'}};
        if (open_br.count(ch)) {
            char target = match.at(ch);
            int depth = 0;
            for (int row = cy; row < (int)lines.size(); ++row) {
                int start = (row == cy) ? cx+1 : 0;
                const auto& line = lines[row];
                for (int col = start; col < (int)line.size(); ++col) {
                    char c = line[col];
                    if (c == ch) depth++;
                    else if (c == target) {
                        if (depth == 0) return {row, col};
                        depth--;
                    }
                }
            }
        } else if (close_br.count(ch)) {
            char target = match.at(ch);
            int depth = 0;
            for (int row = cy; row >= 0; --row) {
                int end = (row == cy) ? cx : lines[row].size();
                for (int col = end-1; col >= 0; --col) {
                    char c = lines[row][col];
                    if (c == ch) depth++;
                    else if (c == target) {
                        if (depth == 0) return {row, col};
                        depth--;
                    }
                }
            }
        }
        return {-1,-1};
    }

    // hex mode
    std::string get_hex_line(int line_idx) const {
        if (line_idx < 0 || line_idx >= (int)lines.size()) return "";
        const std::string& s = lines[line_idx];
        std::string hex;
        for (unsigned char c : s) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02x ", c);
            hex += buf;
        }
        return hex;
    }

    void toggle_hex_mode() { hex_mode = !hex_mode; }

private:
    std::string get_lang(const std::string& path) {
        static const std::map<std::string,std::string> ext_lang = {
            {".js","js"},{".ts","js"},{".py","py"},{".json","json"},{".md","md"},
            {".cpp","cpp"},{".c","c"},{".h","c"},{".txt","text"}
        };
        auto ext = fs::path(path).extension().string();
        auto it = ext_lang.find(ext);
        return (it != ext_lang.end()) ? it->second : "text";
    }

    void detect_indent() {
        int tabs=0, spaces=0;
        std::map<int,int> sizes;
        for (int i=0; i<std::min(200, (int)lines.size()); ++i) {
            const auto& line = lines[i];
            if (line.empty()) continue;
            if (line[0] == '\t') tabs++;
            else if (line[0] == ' ') {
                spaces++;
                int n = line.find_first_not_of(' ');
                if (n > 0) sizes[n]++;
            }
        }
        indent_char = (tabs > spaces) ? '\t' : ' ';
        if (!sizes.empty()) {
            int best = 0, best_count = 0;
            for (auto& p : sizes) if (p.second > best_count) { best_count=p.second; best=p.first; }
            indent_size = best;
        } else indent_size = 4;
    }
};

// ----------------------------------------------------------------------
// File Tree
// ----------------------------------------------------------------------
class FileTree {
public:
    fs::path root;
    struct Item { fs::path path; bool is_dir; bool expanded; int depth; };
    std::vector<Item> items;
    int selected = 0, scroll = 0;
    std::set<std::string> open_set;

    FileTree(const std::string& r) : root(r) { refresh(); }

    void refresh() {
        items.clear();
        walk(root, 0);
        selected = std::min(selected, std::max(0, (int)items.size()-1));
    }

    void walk(const fs::path& dir, int depth) {
        try {
            std::vector<fs::directory_entry> entries;
            for (auto& e : fs::directory_iterator(dir)) entries.push_back(e);
            std::sort(entries.begin(), entries.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    bool ad = a.is_directory(), bd = b.is_directory();
                    if (ad != bd) return ad > bd;
                    return a.path().filename() < b.path().filename();
                });
            for (auto& e : entries) {
                auto name = e.path().filename().string();
                if (name[0] == '.' || name == "node_modules" || name == "__pycache__" || name == ".git") continue;
                bool is_dir = e.is_directory();
                bool expanded = open_set.count(e.path().string()) > 0;
                items.push_back({e.path(), is_dir, expanded, depth});
                if (is_dir && expanded) walk(e.path(), depth+1);
            }
        } catch (...) {}
    }

    void toggle() {
        if (items.empty()) return;
        auto& item = items[selected];
        if (!item.is_dir) return;
        std::string key = item.path.string();
        if (open_set.count(key)) open_set.erase(key);
        else open_set.insert(key);
        refresh();
    }

    void go_up() {
        if (root.parent_path() == root) return;
        root = root.parent_path();
        selected = scroll = 0;
        open_set.clear();
        refresh();
    }

    void move(int d) {
        selected = std::max(0, std::min((int)items.size()-1, selected + d));
    }

    Item* current() {
        if (items.empty()) return nullptr;
        return &items[selected];
    }
};

// ----------------------------------------------------------------------
// Smart Bar (input overlay)
// ----------------------------------------------------------------------
class SmartBar {
public:
    enum Mode { NONE, SEARCH, PATH, CMD, OPEN, GREP, REPLACE, OUTLINE, GOTO };
    Mode mode = NONE;
    std::string text, replace_text;
    int replace_field = 0;
    std::vector<std::string> results;
    int idx = 0;

    void open(Mode m, const std::string& prefill = "") {
        mode = m;
        text = prefill;
        replace_text.clear();
        replace_field = 0;
        results.clear();
        idx = 0;
    }

    void close() { mode = NONE; }

    bool active() const { return mode != NONE; }

    void type(char ch) {
        if (mode == REPLACE && replace_field == 1) replace_text.push_back(ch);
        else text.push_back(ch);
    }

    void backspace() {
        if (mode == REPLACE && replace_field == 1) {
            if (!replace_text.empty()) replace_text.pop_back();
        } else {
            if (!text.empty()) text.pop_back();
        }
    }

    void nav(int d) {
        if (!results.empty()) idx = (idx + d + results.size()) % results.size();
    }

    void update(const Editor& ed, const fs::path& root) {
        std::string q = text;
        std::transform(q.begin(), q.end(), q.begin(), ::tolower);
        results.clear();
        if (mode == SEARCH) {
            for (int i=0; i<(int)ed.lines.size(); ++i) {
                if (q.empty() || std::search(ed.lines[i].begin(), ed.lines[i].end(),
                                             q.begin(), q.end(),
                                             [](char a, char b){ return std::tolower(a)==std::tolower(b); }) != ed.lines[i].end())
                    results.push_back(std::to_string(i+1) + "  " + ed.lines[i].substr(0, 60));
            }
        } else if (mode == CMD) {
            // predefined commands
            static const std::vector<std::pair<std::string,std::string>> commands = {
                {"save","write buffer"}, {"new","empty buffer"}, {"help","keybindings"},
                {"outline","jump to symbol"}, {"git status","short status"},
                {"git log","last commits"}, {"zen","toggle zen"}, {"exit","quit"}
            };
            for (auto& cmd : commands) {
                if (q.empty() || std::search(cmd.first.begin(), cmd.first.end(),
                                             q.begin(), q.end(),
                                             [](char a, char b){ return std::tolower(a)==std::tolower(b); }) != cmd.first.end())
                    results.push_back(cmd.first + "  " + cmd.second);
            }
        } else if (mode == OPEN) {
            try {
                for (auto& p : fs::recursive_directory_iterator(root)) {
                    if (p.is_regular_file()) {
                        auto name = p.path().filename().string();
                        std::string low = name;
                        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
                        if (q.empty() || low.find(q) != std::string::npos) {
                            results.push_back(p.path().string());
                            if (results.size() >= 50) break;
                        }
                    }
                }
            } catch (...) {}
        } else if (mode == GREP && !q.empty()) {
            // use grep external
            std::string cmd = "grep -rn --include=* --color=never -I " + q + " " + root.string() + " 2>/dev/null | head -100";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                char buf[1024];
                while (fgets(buf, sizeof(buf), pipe)) {
                    std::string line(buf);
                    line.pop_back(); // remove newline
                    // strip root prefix
                    auto pos = line.find(root.string());
                    if (pos != std::string::npos) line.replace(pos, root.string().size(), "");
                    results.push_back(line);
                }
                pclose(pipe);
            }
        } else if (mode == GOTO) {
            if (!q.empty()) {
                try {
                    int ln = std::stoi(q);
                    if (ln >=1 && ln <= (int)ed.lines.size())
                        results.push_back(std::to_string(ln) + "  " + ed.lines[ln-1].substr(0, 60));
                    else
                        results.push_back("line out of range (1-" + std::to_string(ed.lines.size()) + ")");
                } catch (...) {
                    results.push_back("enter a line number");
                }
            }
        } else if (mode == OUTLINE) {
            // simple outline for Python/JS
            // this is a stub; full implementation would parse.
            for (int i=0; i<(int)ed.lines.size(); ++i) {
                auto& l = ed.lines[i];
                std::string stripped = l;
                stripped.erase(0, stripped.find_first_not_of(" \t"));
                if (stripped.rfind("def ", 0)==0 || stripped.rfind("class ", 0)==0 ||
                    stripped.rfind("function ", 0)==0 || stripped.rfind("const ", 0)==0) {
                    std::string label = stripped.substr(0, stripped.find('('));
                    if (q.empty() || std::search(label.begin(), label.end(), q.begin(), q.end(),
                                                 [](char a, char b){ return std::tolower(a)==std::tolower(b); }) != label.end())
                        results.push_back(std::to_string(i+1) + "  " + label);
                }
            }
        } else if (mode == REPLACE) {
            if (!text.empty()) {
                for (int i=0; i<(int)ed.lines.size(); ++i) {
                    if (ed.lines[i].find(text) != std::string::npos)
                        results.push_back(std::to_string(i+1) + "  " + ed.lines[i].substr(0, 58));
                }
            }
        }
        idx = std::min(idx, (int)results.size()-1);
    }

    int hit_line() const {
        if (results.empty() || (mode != SEARCH && mode != OUTLINE && mode != REPLACE)) return -1;
        try {
            return std::stoi(results[idx].substr(0, results[idx].find(' '))) - 1;
        } catch (...) { return -1; }
    }

    std::string hit_cmd() const {
        if (mode == CMD && !results.empty()) {
            auto pos = results[idx].find(' ');
            if (pos != std::string::npos) return results[idx].substr(0, pos);
        }
        return text;
    }

    std::string hit_file() const {
        if (mode == OPEN && !results.empty()) return results[idx];
        return "";
    }

    // returns {filepath, line_number}
    std::pair<std::string,int> hit_grep() const {
        if (mode == GREP && !results.empty()) {
            auto& r = results[idx];
            size_t colon1 = r.find(':');
            if (colon1 != std::string::npos) {
                size_t colon2 = r.find(':', colon1+1);
                if (colon2 != std::string::npos) {
                    std::string f = r.substr(0, colon1);
                    int ln = std::stoi(r.substr(colon1+1, colon2-colon1-1));
                    return {f, ln-1};
                }
            }
        }
        return {"", -1};
    }
};

// ----------------------------------------------------------------------
// Main application
// ----------------------------------------------------------------------
class XcXApp {
public:
    Editor ed;
    FileTree tree;
    SmartBar bar;
    bool running = true;
    bool zen = false;
    bool help_screen = false;
    bool focus_explorer = false;
    bool show_nano_bar = true;
    std::string msg;
    bool msg_err = false;

    // terminal dimensions
    int rows, cols;
    int sidebar_w = 26;
    int gutter_w = 1;
    int lnum_w = 5;

    XcXApp(const std::string& root) : tree(root) {
        ed = Editor();
        getmaxyx(stdscr, rows, cols);
    }

    int editor_height() const {
        int extra = (zen || !show_nano_bar) ? 2 : 3;
        int h = rows - extra;
        return std::max(4, h);
    }

    int editor_width() const {
        return std::max(10, cols - sidebar_w - gutter_w - lnum_w - 2);
    }

    int editor_start() const {
        return sidebar_w + gutter_w + 1 + lnum_w;
    }

    void refresh_git() {
        // stub: we could call git diff here
    }

    void render() {
        clear();
        // Title bar
        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        std::string title = " XcX Editor ";
        if (!ed.filepath.empty()) {
            title += "— " + fs::path(ed.filepath).filename().string();
            if (ed.modified) title += "*";
        }
        if (zen) title += " [zen]";
        if (ed.hex_mode) title += " [HEX]";
        mvprintw(0, 0, "%-*s", cols, title.c_str());
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

        int EH = editor_height();
        int EW = editor_width();
        int ES = editor_start();

        // Sidebar (file tree)
        if (!zen) {
            // Sidebar header
            attron(COLOR_PAIR(focus_explorer ? CP_SIDEBAR_SEL : CP_SIDEBAR));
            mvprintw(1, 0, "%-*s", sidebar_w, " EXPLORER");
            attroff(COLOR_PAIR(focus_explorer ? CP_SIDEBAR_SEL : CP_SIDEBAR));

            // Tree items
            for (int i=0; i<EH-1; ++i) {
                int idx = tree.scroll + i;
                if (idx >= (int)tree.items.size()) {
                    mvprintw(i+2, 0, "%-*s", sidebar_w, "");
                    continue;
                }
                auto& item = tree.items[idx];
                bool sel = (idx == tree.selected && focus_explorer);
                std::string label = std::string(item.depth*2, ' ') +
                                    (item.is_dir ? (item.expanded ? "▾ " : "▸ ") : "  ") +
                                    item.path.filename().string();
                if ((int)label.size() > sidebar_w-1) label = label.substr(0, sidebar_w-1);
                if (sel) attron(COLOR_PAIR(CP_SIDEBAR_SEL) | A_BOLD);
                else attron(COLOR_PAIR(CP_SIDEBAR));
                mvprintw(i+2, 0, "%-*s", sidebar_w, label.c_str());
                if (sel) attroff(COLOR_PAIR(CP_SIDEBAR_SEL) | A_BOLD);
                else attroff(COLOR_PAIR(CP_SIDEBAR));
            }

            // Git gutter (1 char) – just placeholder
            for (int i=0; i<EH; ++i) {
                int ln = ed.sy + i + 1;
                // Stub: we could show git markers
                mvprintw(i+1, sidebar_w, " ");
            }

            // Divider
            int div_col = sidebar_w + gutter_w + 1;
            for (int r=1; r<rows-1; ++r) {
                mvprintw(r, div_col, "|");
            }
        }

        // Line numbers
        int ln_col = zen ? 1 : (sidebar_w + gutter_w + 1 + 1);
        for (int i=0; i<EH; ++i) {
            int ln = ed.sy + i + 1;
            if (ln <= (int)ed.lines.size()) {
                bool cur = (ed.sy + i == ed.cy);
                attron(COLOR_PAIR(cur ? CP_LNUM_CUR : CP_LNUM));
                mvprintw(i+1, ln_col, "%*d ", lnum_w-1, ln);
                attroff(COLOR_PAIR(cur ? CP_LNUM_CUR : CP_LNUM));
            } else {
                mvprintw(i+1, ln_col, "%*s", lnum_w, "");
            }
        }

        // Editor content
        if (help_screen) {
            static const std::vector<std::string> help_lines = {
                "XcX Editor – keybindings",
                "Ctrl+S save   Ctrl+Q quit   Ctrl+Z undo   Ctrl+Y redo",
                "Ctrl+K cut line/selection   Ctrl+U paste   Alt+6 copy",
                "Ctrl+A mark   Ctrl+C cursor pos   Ctrl+G help",
                "Ctrl+F grep   Ctrl+P quick open   Ctrl+O outline",
                "Ctrl+H toggle hex mode   F5 explorer   F11 zen",
                "↑↓←→ move   Home/End   PgUp/PgDn   Ctrl+←/→ word",
                "Alt+S search   Alt+R replace   Alt+W folder",
                "any key close"
            };
            for (int i=0; i<std::min(EH, (int)help_lines.size()); ++i) {
                mvprintw(i+1, ES, "%-*s", EW, help_lines[i].c_str());
            }
        } else {
            // Draw editor lines
            for (int i=0; i<EH; ++i) {
                int li = ed.sy + i;
                if (li >= (int)ed.lines.size()) {
                    mvprintw(i+1, ES, "%-*s", EW, "");
                    continue;
                }
                std::string line = ed.lines[li];
                // If hex mode, show hex representation
                if (ed.hex_mode) {
                    std::string hex = ed.get_hex_line(li);
                    // also show ASCII
                    std::string ascii;
                    for (char c : line) ascii += (std::isprint(c) ? c : '.');
                    std::string combined = hex + "  " + ascii;
                    if ((int)combined.size() > EW) combined = combined.substr(0, EW);
                    mvprintw(i+1, ES, "%-*s", EW, combined.c_str());
                } else {
                    // Highlight (stub)
                    std::string hl = highlight_line(line, ed.lang);
                    // Truncate to fit
                    if ((int)hl.size() > EW) hl = hl.substr(0, EW);
                    mvprintw(i+1, ES, "%-*s", EW, hl.c_str());
                }

                // Selection highlight (if any) – we'll just use reverse video for selected region
                auto sel = ed.get_selection();
                if (!sel.empty() && !ed.hex_mode) {
                    int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
                    if (li >= sy && li <= ey) {
                        int start = (li == sy) ? sx : 0;
                        int end = (li == ey) ? ex : ed.lines[li].size();
                        // We'll just draw the selection with reverse video over the printed text.
                        // Since we already printed, we'll reposition and highlight.
                        // Simpler: we don't implement full selection highlighting in this C++ version.
                        // We'll just rely on the cursor to indicate selection.
                    }
                }
            }

            // Cursor for text mode
            if (!help_screen && !bar.active() && !ed.hex_mode) {
                int sr = ed.cy - ed.sy + 1;
                int sc = ed.cx - ed.sx + ES;
                if (sr >= 1 && sr <= EH && sc >= ES && sc < ES+EW) {
                    attron(COLOR_PAIR(CP_EDITOR_CUR) | A_REVERSE);
                    mvprintw(sr, sc, "%c", (ed.cy < (int)ed.lines.size() && ed.cx < (int)ed.lines[ed.cy].size()) ? ed.lines[ed.cy][ed.cx] : ' ');
                    attroff(COLOR_PAIR(CP_EDITOR_CUR) | A_REVERSE);
                }
                // Bracket match highlight
                auto match = ed.find_bracket_match();
                if (match.first != -1) {
                    int br = match.first - ed.sy + 1;
                    int bc = match.second - ed.sx + ES;
                    if (br >=1 && br <= EH && bc >= ES && bc < ES+EW) {
                        attron(COLOR_PAIR(CP_BRACKET_MATCH));
                        mvprintw(br, bc, "%c", ed.lines[match.first][match.second]);
                        attroff(COLOR_PAIR(CP_BRACKET_MATCH));
                    }
                }
            } else if (ed.hex_mode && !help_screen && !bar.active()) {
                // *** Hex mode cursor ***
                int sr = ed.cy - ed.sy + 1;
                if (sr >= 1 && sr <= EH) {
                    // Get the hex string for the current line
                    std::string hex = ed.get_hex_line(ed.cy);
                    // Byte index = ed.cx (should be within line size)
                    int byte_idx = ed.cx;
                    if (byte_idx >= 0 && byte_idx < (int)ed.lines[ed.cy].size()) {
                        // Each byte takes 3 chars: two hex digits + space
                        int col_offset = byte_idx * 3;
                        if (col_offset + 2 < (int)hex.size()) {
                            int sc = ES + col_offset;
                            if (sc < ES + EW) {
                                // Highlight the three characters (two hex digits + space) with reverse video
                                attron(COLOR_PAIR(CP_EDITOR_CUR) | A_REVERSE);
                                mvprintw(sr, sc, "%c%c%c", hex[col_offset], hex[col_offset+1], hex[col_offset+2]);
                                attroff(COLOR_PAIR(CP_EDITOR_CUR) | A_REVERSE);
                            }
                        }
                    }
                }
            }

            // Sticky scroll (if scrolled)
            if (!zen && ed.sy > 0) {
                // Show the function/class header at top
                // Stub: find last def/class above viewport
                int sticky = -1;
                for (int i=ed.sy-1; i>=0; --i) {
                    auto& l = ed.lines[i];
                    std::string s = l;
                    s.erase(0, s.find_first_not_of(" \t"));
                    if (s.rfind("def ",0)==0 || s.rfind("class ",0)==0 || s.rfind("function ",0)==0) {
                        sticky = i; break;
                    }
                }
                if (sticky != -1) {
                    std::string stub = ed.lines[sticky].substr(0, EW);
                    attron(COLOR_PAIR(CP_STICKY) | A_DIM);
                    mvprintw(1, ES, "%-*s", EW, stub.c_str());
                    attroff(COLOR_PAIR(CP_STICKY) | A_DIM);
                }
            }
        }

        // Status bar
        if (!zen) {
            // Nano-style shortcut bar
            if (show_nano_bar) {
                attron(COLOR_PAIR(CP_DEFAULT));
                mvprintw(rows-2, 0, "%-*s", cols, 
                         (focus_explorer ? "F1 Help  ↑↓ Nav  Enter Open  Backspace Up  Esc Editor" :
                          "F1 Help  ^O Save  ^W Search  ^K Cut  ^U Paste  ^A Mark  ^C Pos  ^Q Exit"));
                attroff(COLOR_PAIR(CP_DEFAULT));
            }

            // Status line
            attron(COLOR_PAIR(CP_STATUS));
            std::string mode_str = focus_explorer ? "EXPLORE" : (ed.hex_mode ? "HEX" : "INSERT");
            std::string fname = ed.filepath.empty() ? "[New File]" : fs::path(ed.filepath).filename().string();
            if (ed.modified) fname += "*";
            std::string left = " " + mode_str + "  " + fname;
            std::string right = " Ln " + std::to_string(ed.cy+1) + ", Col " + std::to_string(ed.cx+1) +
                                "  " + std::to_string(ed.lines.size()) + " lines  " +
                                (ed.indent_char == '\t' ? "tab" : std::to_string(ed.indent_size)+"spc") +
                                "  " + ed.lang;
            int mid = cols - left.size() - right.size() - 2;
            if (mid < 0) mid = 0;
            std::string center = msg;
            if ((int)center.size() > mid) center = center.substr(0, mid);
            attron(COLOR_PAIR(msg_err ? CP_STATUS_MSG : CP_STATUS));
            mvprintw(rows-1, 0, "%s%-*s%s", left.c_str(), mid, center.c_str(), right.c_str());
            attroff(COLOR_PAIR(msg_err ? CP_STATUS_MSG : CP_STATUS));
        }

        // SmartBar overlay
        if (bar.active()) {
            draw_bar();
        }

        refresh();
    }

    void draw_bar() {
        // Simplified drawing of the input bar
        int w = cols - 4;
        int x = 2, y = rows/2 - 4;
        std::string title;
        switch (bar.mode) {
            case SmartBar::SEARCH:   title = " SEARCH "; break;
            case SmartBar::PATH:     title = " GO TO FOLDER "; break;
            case SmartBar::CMD:      title = " COMMAND PALETTE "; break;
            case SmartBar::OPEN:     title = " QUICK OPEN "; break;
            case SmartBar::GREP:     title = " GREP "; break;
            case SmartBar::REPLACE:  title = " FIND + REPLACE "; break;
            case SmartBar::OUTLINE:  title = " OUTLINE "; break;
            case SmartBar::GOTO:     title = " GO TO LINE "; break;
            default: return;
        }
        // Draw box with title
        attron(COLOR_PAIR(CP_BAR));
        mvprintw(y, x, "┌─%s─┐", title.c_str());
        int iw = w - 2;
        if (bar.mode == SmartBar::REPLACE) {
            std::string find = "find:    " + bar.text + (bar.replace_field==0 ? "_" : "");
            std::string repl = "replace: " + bar.replace_text + (bar.replace_field==1 ? "_" : "");
            mvprintw(y+1, x, "│ %-*s │", iw-2, find.c_str());
            mvprintw(y+2, x, "│ %-*s │", iw-2, repl.c_str());
            mvprintw(y+3, x, "├─%s─┤", std::string(iw-2, '─').c_str());
            int row = y+4;
            for (int i=0; i<std::min(5, (int)bar.results.size()); ++i) {
                bool sel = (i == bar.idx);
                attron(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
                mvprintw(row+i, x, "│ %-*s │", iw-2, bar.results[i].c_str());
                attroff(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
            }
        } else {
            std::string input = bar.text + "_";
            mvprintw(y+1, x, "│ %-*s │", iw-2, input.c_str());
            mvprintw(y+2, x, "├─%s─┤", std::string(iw-2, '─').c_str());
            int row = y+3;
            for (int i=0; i<std::min(5, (int)bar.results.size()); ++i) {
                bool sel = (i == bar.idx);
                attron(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
                mvprintw(row+i, x, "│ %-*s │", iw-2, bar.results[i].c_str());
                attroff(sel ? COLOR_PAIR(CP_BAR_SEL) : COLOR_PAIR(CP_BAR));
            }
        }
        attroff(COLOR_PAIR(CP_BAR));
        // Hint line
        std::string hint;
        switch (bar.mode) {
            case SmartBar::SEARCH: hint = "↑↓ select, Enter jump, Esc cancel"; break;
            case SmartBar::GOTO: hint = "Enter go, Esc cancel"; break;
            case SmartBar::REPLACE: hint = "Tab switch fields, Enter replace all, Esc cancel"; break;
            default: hint = "Enter confirm, Esc cancel";
        }
        mvprintw(rows/2+3, x, "│ %-*s │", iw-2, hint.c_str());
        mvprintw(rows/2+4, x, "└─%s─┘", std::string(iw-2, '─').c_str());
    }

    void handle_key(int key) {
        if (help_screen) { help_screen = false; return; }

        // F9 chord? We'll ignore for simplicity.

        // Terminal focus? Not implemented.

        // SmartBar
        if (bar.active()) {
            switch (key) {
                case 27: bar.close(); break;
                case '\n': case '\r': bar_confirm(); break;
                case KEY_UP: bar.nav(-1); break;
                case KEY_DOWN: bar.nav(1); break;
                case '\t':
                    if (bar.mode == SmartBar::REPLACE) bar.replace_field = 1 - bar.replace_field;
                    break;
                case KEY_BACKSPACE: case 127:
                    bar.backspace();
                    bar.update(ed, tree.root);
                    break;
                default:
                    if (key >= 32 && key <= 126) { bar.type((char)key); bar.update(ed, tree.root); }
                    break;
            }
            return;
        }

        // Global shortcuts
        if (key == KEY_F(5)) { focus_explorer = true; msg = "explorer"; return; }
        if (key == KEY_F(6)) { focus_explorer = false; msg = "editor"; return; }
        if (key == KEY_F(11)) { zen = !zen; msg = zen ? "zen on" : "zen off"; return; }
        if (key == '\x08') { ed.undo(); msg = "undo"; return; } // Ctrl+H? Actually Ctrl+H is backspace, but we map to hex toggle later.
        // Ctrl+Q: quit
        if (key == 17) { running = false; return; }
        // Ctrl+S: save
        if (key == 19) {
            if (ed.save()) { refresh_git(); msg = "saved " + fs::path(ed.filepath).filename().string(); }
            else msg = "save failed";
            return;
        }
        // Ctrl+G: help
        if (key == 7) { help_screen = true; return; }
        // Ctrl+Z undo, Ctrl+Y redo
        if (key == 26) { ed.undo(); msg = "undo"; return; }
        if (key == 25) { ed.redo(); msg = "redo"; return; }
        // Ctrl+A: mark
        if (key == 1) { ed.toggle_mark(); msg = ed.mark_active ? "Mark set" : "Mark unset"; return; }
        // Ctrl+K: cut
        if (key == 11) {
            auto sel = ed.get_selection();
            if (!sel.empty()) { ed.cut_selection(); msg = "cut selection"; }
            else { ed.cut_line(); msg = "cut line"; }
            return;
        }
        // Ctrl+U: paste
        if (key == 21) { ed.paste(); msg = "paste"; return; }
        // Alt+6: copy (ESC+6)
        if (key == 27) { // ESC
            int next = getch();
            if (next == '6' || next == 'L') { ed.copy_selection(); msg = "copied"; }
            else { ungetch(next); } // not handled
            return;
        }
        // Ctrl+_ (control+underscore) – we'll treat as Ctrl+_ = 31? Actually Ctrl+_ is 0x1F.
        if (key == 31) {
            bar.open(SmartBar::GOTO, std::to_string(ed.cy+1));
            bar.update(ed, tree.root);
            return;
        }
        // Ctrl+C: show cursor pos
        if (key == 3) {
            msg = "line " + std::to_string(ed.cy+1) + "/" + std::to_string(ed.lines.size()) +
                  ", col " + std::to_string(ed.cx+1);
            return;
        }
        // Ctrl+F: grep
        if (key == 6) { bar.open(SmartBar::GREP); bar.update(ed, tree.root); return; }
        // Ctrl+P: quick open
        if (key == 16) { bar.open(SmartBar::OPEN); bar.update(ed, tree.root); return; }
        // Ctrl+O: outline
        if (key == 15) { bar.open(SmartBar::OUTLINE); bar.update(ed, tree.root); return; }
        // Alt+S: search
        if (key == 27) {
            int next = getch();
            if (next == 's' || next == 'S') { bar.open(SmartBar::SEARCH); bar.update(ed, tree.root); }
            else if (next == 'r' || next == 'R') { bar.open(SmartBar::REPLACE); bar.update(ed, tree.root); }
            else if (next == 'w' || next == 'W') { bar.open(SmartBar::PATH, tree.root.string()); bar.update(ed, tree.root); }
            else { ungetch(next); }
            return;
        }
        // Ctrl+H: toggle hex mode
        if (key == 8) { ed.toggle_hex_mode(); msg = ed.hex_mode ? "Hex mode" : "Text mode"; return; }

        // Explorer
        if (focus_explorer) {
            switch (key) {
                case KEY_UP: tree.move(-1); break;
                case KEY_DOWN: tree.move(1); break;
                case KEY_BACKSPACE: case 127: tree.go_up(); msg = "up " + tree.root.string(); break;
                case '\n': case '\r': {
                    auto* item = tree.current();
                    if (item) {
                        if (item->is_dir) { tree.toggle(); }
                        else {
                            ed.load(item->path.string());
                            refresh_git();
                            msg = "opened " + item->path.filename().string();
                            focus_explorer = false;
                        }
                    }
                    break;
                }
                case 27: focus_explorer = false; break; // ESC
                default: break;
            }
            return;
        }

        // Editor
        if (!focus_explorer) {
            switch (key) {
                case KEY_UP: ed.move(-1,0); break;
                case KEY_DOWN: ed.move(1,0); break;
                case KEY_LEFT: ed.move(0,-1); break;
                case KEY_RIGHT: ed.move(0,1); break;
                case KEY_HOME: ed.home(); break;
                case KEY_END: ed.end(); break;
                case KEY_PPAGE: ed.page(editor_height()-2, -1); break;
                case KEY_NPAGE: ed.page(editor_height()-2, 1); break;
                case '\n': case '\r': ed.newline(); break;
                case KEY_BACKSPACE: case 127: ed.backspace(); break;
                case KEY_DC: ed.delete_fwd(); break;
                case '\t': ed.tab(); break;
                case 27: { // ESC sequence: word left/right?
                    int next = getch();
                    if (next == '[') {
                        next = getch();
                        if (next == '1') {
                            next = getch(); // ';'
                            if (next == ';') {
                                next = getch();
                                if (next == '5') {
                                    next = getch();
                                    if (next == 'D') { ed.word_left(); }
                                    else if (next == 'C') { ed.word_right(); }
                                    else { ungetch(next); ungetch(';'); ungetch('1'); ungetch('['); ungetch(27); }
                                } else { ungetch(next); ungetch(';'); ungetch('1'); ungetch('['); ungetch(27); }
                            } else { ungetch(next); ungetch('1'); ungetch('['); ungetch(27); }
                        } else { ungetch(next); ungetch('['); ungetch(27); }
                    } else { ungetch(next); ungetch(27); }
                    break;
                }
                default:
                    if (key >= 32 && key <= 126) {
                        ed.insert_char((char)key);
                    }
                    break;
            }
        }
    }

    void bar_confirm() {
        switch (bar.mode) {
            case SmartBar::GOTO: {
                try {
                    int ln = std::stoi(bar.text);
                    if (ed.go_to_line(ln)) msg = "line " + std::to_string(ln);
                    else msg = "invalid line";
                } catch (...) { msg = "invalid"; }
                bar.close();
                break;
            }
            case SmartBar::SEARCH:
            case SmartBar::OUTLINE: {
                int ln = bar.hit_line();
                if (ln >= 0) { ed.cy = ln; ed.cx = 0; msg = "line " + std::to_string(ln+1); }
                bar.close();
                break;
            }
            case SmartBar::PATH: {
                fs::path p(bar.text);
                if (fs::is_directory(p)) {
                    tree.root = p;
                    tree.selected = tree.scroll = 0;
                    tree.open_set.clear();
                    tree.refresh();
                    focus_explorer = true;
                    msg = "→ " + p.string();
                } else msg = "not a directory";
                bar.close();
                break;
            }
            case SmartBar::OPEN: {
                std::string f = bar.hit_file();
                if (!f.empty()) {
                    ed.load(f);
                    refresh_git();
                    msg = "opened " + fs::path(f).filename().string();
                    focus_explorer = false;
                }
                bar.close();
                break;
            }
            case SmartBar::GREP: {
                auto hit = bar.hit_grep();
                if (!hit.first.empty()) {
                    fs::path full = tree.root / hit.first;
                    if (!fs::exists(full)) full = hit.first;
                    if (fs::exists(full)) {
                        ed.load(full.string());
                        refresh_git();
                        ed.cy = hit.second;
                        ed.cx = 0;
                        msg = "opened " + full.filename().string() + ":" + std::to_string(hit.second+1);
                        focus_explorer = false;
                    }
                }
                bar.close();
                break;
            }
            case SmartBar::REPLACE: {
                std::string find = bar.text, repl = bar.replace_text;
                if (!find.empty()) {
                    int count = 0;
                    for (auto& line : ed.lines) {
                        size_t pos = 0;
                        while ((pos = line.find(find, pos)) != std::string::npos) {
                            line.replace(pos, find.size(), repl);
                            pos += repl.size();
                            count++;
                        }
                    }
                    if (count > 0) { ed.snapshot(); ed.modified = true; msg = "replaced " + std::to_string(count); }
                    else msg = "no matches";
                }
                bar.close();
                break;
            }
            case SmartBar::CMD: {
                std::string cmd = bar.hit_cmd();
                bar.close();
                if (cmd == "save") { if (ed.save()) { refresh_git(); msg = "saved"; } else msg = "save failed"; }
                else if (cmd == "new") { ed.lines = {""}; ed.filepath = ""; ed.cy=ed.cx=0; ed.modified=true; msg="new buffer"; }
                else if (cmd == "help") help_screen = true;
                else if (cmd == "zen") { zen = !zen; msg = zen ? "zen on" : "zen off"; }
                else if (cmd == "outline") { bar.open(SmartBar::OUTLINE); bar.update(ed, tree.root); }
                else if (cmd == "exit") running = false;
                else msg = "unknown command";
                break;
            }
            default: bar.close();
        }
    }

    void run() {
        // Load initial file if given as argument
        // We'll use global argv.
        // In main we'll instantiate with arguments.
        while (running) {
            render();
            int ch = getch();
            if (ch != ERR) {
                msg_err = false;
                msg = "";
                handle_key(ch);
            }
        }
    }
};

// ----------------------------------------------------------------------
// main
// ----------------------------------------------------------------------
int main(int argc, char** argv) {
    // Initialize ncurses
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // hide cursor
    if (has_colors()) init_colors();

    std::string root = ".";
    std::string file = "";
    if (argc > 1) {
        fs::path p(argv[1]);
        if (fs::exists(p)) {
            if (fs::is_directory(p)) root = p.string();
            else { file = p.string(); root = p.parent_path().string(); }
        }
    }

    XcXApp app(root);
    if (!file.empty()) {
        app.ed.load(file);
        app.refresh_git();
        app.focus_explorer = false;
    } else {
        // Start with a blank or first file in tree?
    }

    app.run();

    // Cleanup
    endwin();
    return 0;
}
