#include "editor.h"
#include <cstring>

// ----------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------
Editor::Editor() {
    lines.push_back("");
}

// ----------------------------------------------------------------------
// Undo/Redo snapshot
// ----------------------------------------------------------------------
void Editor::snapshot() {
    Snapshot s{lines, cy, cx};
    if (!undo_stack.empty() &&
        undo_stack.back().lines == s.lines &&
        undo_stack.back().cy == s.cy &&
        undo_stack.back().cx == s.cx)
        return;
    undo_stack.push_back(std::move(s));
    if (undo_stack.size() > UNDO_LIMIT)
        undo_stack.pop_front();
    redo_stack.clear();
}

void Editor::undo() {
    if (undo_stack.empty()) return;
    redo_stack.push_back({lines, cy, cx});
    auto s = std::move(undo_stack.back());
    undo_stack.pop_back();
    lines = std::move(s.lines);
    cy = s.cy;
    cx = s.cx;
    modified = true;
    clamp_cursor();
}

void Editor::redo() {
    if (redo_stack.empty()) return;
    undo_stack.push_back({lines, cy, cx});
    auto s = std::move(redo_stack.back());
    redo_stack.pop_back();
    lines = std::move(s.lines);
    cy = s.cy;
    cx = s.cx;
    modified = true;
    clamp_cursor();
}

// ----------------------------------------------------------------------
// File I/O
// ----------------------------------------------------------------------
void Editor::load(const std::string& path) {
    filepath = path;
    lang = detect_lang(path);
    std::ifstream f(path);
    if (!f) { lines = {""}; cy = cx = sy = sx = 0; modified = true; return; }
    lines.clear();
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    cy = cx = sy = sx = 0;
    modified = false;
    undo_stack.clear();
    redo_stack.clear();
    detect_indent();
    mark_active = false;
    clear_search_highlights();
}

bool Editor::save() {
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

void Editor::new_buffer() {
    lines = {""};
    filepath.clear();
    lang = "text";
    cy = cx = sy = sx = 0;
    modified = true;
    undo_stack.clear();
    redo_stack.clear();
    mark_active = false;
    clear_search_highlights();
}

// ----------------------------------------------------------------------
// Editing operations
// ----------------------------------------------------------------------
void Editor::insert_char(char ch) {
    snapshot();
    auto& l = lines[cy];

    // Bracket auto-close
    static const std::map<char,char> pairs = {
        {'(',')'},{'[',']'},{'{','}'},
        {'"','"'},{'\'','\''},{'`','`'}
    };
    static const std::set<char> closing = {')',']','}','"','\'','`'};

    // Skip-over closing char
    if (closing.count(ch) && cx < (int)l.size() && l[cx] == ch) {
        cx++;
        return;
    }

    // Auto-close opening bracket
    if (pairs.count(ch) && !closing.count(ch)) {
        char closing_char = pairs.at(ch);
        l.insert(cx, 1, ch);
        l.insert(cx + 1, 1, closing_char);
        cx++;
        modified = true;
        return;
    }

    // Handle tab
    if (ch == '\t') {
        tab();
        return;
    }

    // Regular char
    l.insert(cx, 1, ch);
    cx++;
    modified = true;
}

void Editor::insert_text(const std::string& text) {
    if (text.empty()) return;
    snapshot();
    lines[cy].insert(cx, text);
    cx += text.size();
    modified = true;
}

void Editor::backspace() {
    if (hex_mode) {
        // In hex mode, backspace does nothing (hex edit not fully implemented)
        return;
    }
    snapshot();
    auto& l = lines[cy];

    // Auto-delete bracket pair
    static const std::map<char,char> pairs = {{'(',')'},{'[',']'},{'{','}'}};
    if (cx > 0 && cx < (int)l.size() &&
        pairs.count(l[cx-1]) && l[cx] == pairs.at(l[cx-1])) {
        l.erase(cx - 1, 2);
        cx--;
        modified = true;
        return;
    }

    if (cx > 0) {
        l.erase(cx - 1, 1);
        cx--;
        modified = true;
    } else if (cy > 0) {
        cx = static_cast<int>(lines[cy - 1].size());
        lines[cy - 1] += lines[cy];
        lines.erase(lines.begin() + cy);
        cy--;
        modified = true;
    }
}

void Editor::delete_fwd() {
    if (hex_mode) return;
    snapshot();
    auto& l = lines[cy];
    if (cx < (int)l.size()) {
        l.erase(cx, 1);
        modified = true;
    } else if (cy + 1 < (int)lines.size()) {
        lines[cy] += lines[cy + 1];
        lines.erase(lines.begin() + cy + 1);
        modified = true;
    }
}

void Editor::newline() {
    snapshot();
    auto& l = lines[cy];

    // Calculate auto-indent
    int indent = l.find_first_not_of(" \t");
    if (indent == (int)std::string::npos) indent = static_cast<int>(l.size());

    // If line ends with colon or opening brace, indent deeper
    auto trimmed = l;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    if (!trimmed.empty()) {
        char last = trimmed.back();
        if (last == ':' || last == '{') indent += indent_size;
    }

    // Split the line
    std::string rest = l.substr(cx);
    l.erase(cx);

    std::string spaces(static_cast<size_t>(indent), indent_char == '\t' ? '\t' : ' ');
    lines.insert(lines.begin() + cy + 1, spaces + rest);
    cy++;
    cx = indent;
    modified = true;
}

void Editor::tab() {
    if (hex_mode) return;
    snapshot();
    if (indent_char == '\t') {
        lines[cy].insert(cx, 1, '\t');
        cx++;
    } else {
        int spaces = indent_size - (cx % indent_size);
        lines[cy].insert(static_cast<size_t>(cx), spaces, ' ');
        cx += spaces;
    }
    modified = true;
}

void Editor::untab() {
    if (hex_mode || cx <= 0) return;
    snapshot();
    int amount = cx % indent_size;
    if (amount == 0) amount = indent_size;
    int removed = 0;
    for (int i = 0; i < amount && cx > 0; ++i) {
        if (cx > 0 && lines[cy][cx - 1] == ' ') {
            lines[cy].erase(cx - 1, 1);
            cx--;
            removed++;
        } else {
            break;
        }
    }
    if (removed > 0) modified = true;
}

// ----------------------------------------------------------------------
// Cursor movement
// ----------------------------------------------------------------------
void Editor::move(int dy, int dx) {
    cy = std::max(0, std::min(line_count() - 1, cy + dy));
    if (dx != 0) {
        cx = std::max(0, std::min(static_cast<int>(lines[cy].size()), cx + dx));
    } else {
        cx = std::min(cx, static_cast<int>(lines[cy].size()));
    }
}

void Editor::move_up(int n) { move(-n, 0); }
void Editor::move_down(int n) { move(n, 0); }
void Editor::move_left(int n) { move(0, -n); }
void Editor::move_right(int n) { move(0, n); }

void Editor::home() {
    int indent = lines[cy].find_first_not_of(" \t");
    if (indent == (int)std::string::npos) indent = static_cast<int>(lines[cy].size());
    if (cx == indent) cx = 0;
    else cx = indent;
}

void Editor::end() {
    cx = static_cast<int>(lines[cy].size());
}

void Editor::page(int rows, int dir) {
    cy = std::max(0, std::min(line_count() - 1, cy + dir * rows));
    cx = std::min(cx, static_cast<int>(lines[cy].size()));
}

void Editor::word_left() {
    while (cx > 0 && !std::isalnum(lines[cy][cx - 1])) --cx;
    while (cx > 0 && std::isalnum(lines[cy][cx - 1])) --cx;
}

void Editor::word_right() {
    auto& l = lines[cy];
    while (cx < (int)l.size() && !std::isalnum(l[cx])) ++cx;
    while (cx < (int)l.size() && std::isalnum(l[cx])) ++cx;
}

void Editor::goto_bol() { cx = 0; }
void Editor::goto_eol() { cx = static_cast<int>(lines[cy].size()); }
void Editor::goto_file_begin() { cy = 0; cx = 0; }
void Editor::goto_file_end() { cy = line_count() - 1; cx = 0; }

// ----------------------------------------------------------------------
// Selection
// ----------------------------------------------------------------------
void Editor::toggle_mark() {
    if (mark_active) {
        // If cursor hasn't moved since mark was set, just unset
        mark_active = false;
    } else {
        mark_cy = cy;
        mark_cx = cx;
        mark_active = true;
    }
}

std::vector<int> Editor::get_selection() const {
    if (!mark_active) return {};
    int sy = mark_cy, sx = mark_cx;
    int ey = cy, ex = cx;
    if (sy > ey || (sy == ey && sx > ex)) {
        std::swap(sy, ey);
        std::swap(sx, ex);
    }
    return {sy, sx, ey, ex};
}

std::string Editor::get_selected_text() const {
    auto sel = get_selection();
    if (sel.empty()) return "";
    int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
    if (sy == ey) {
        return lines[sy].substr(sx, ex - sx);
    }
    std::string result = lines[sy].substr(sx);
    for (int i = sy + 1; i < ey; ++i) result += '\n' + lines[i];
    result += '\n' + lines[ey].substr(0, ex);
    return result;
}

bool Editor::cut_selection() {
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
        for (int i = ey; i > sy; --i)
            lines.erase(lines.begin() + i);
        cy = sy; cx = sx;
    }
    mark_active = false;
    modified = true;
    return true;
}

void Editor::cut_line() {
    snapshot();
    clipboard = lines[cy] + '\n';
    if (lines.size() == 1) {
        lines[0].clear();
        cx = 0;
    } else {
        lines.erase(lines.begin() + cy);
        if (cy >= (int)lines.size())
            cy = (int)lines.size() - 1;
        cx = std::min(cx, (int)lines[cy].size());
    }
    mark_active = false;
    modified = true;
}

void Editor::copy_selection() {
    auto txt = get_selected_text();
    if (!txt.empty()) clipboard = txt;
}

void Editor::copy_line() {
    clipboard = lines[cy] + '\n';
}

void Editor::paste() {
    if (clipboard.empty()) return;
    snapshot();

    // Delete selection if active
    if (has_selection()) {
        auto sel = get_selection();
        int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
        if (sy == ey) {
            lines[sy].erase(sx, ex - sx);
            cy = sy; cx = sx;
        } else {
            lines[sy].erase(sx);
            lines[sy] += lines[ey].substr(ex);
            for (int i = ey; i > sy; --i)
                lines.erase(lines.begin() + i);
            cy = sy; cx = sx;
        }
        mark_active = false;
    }

    std::vector<std::string> paste_lines;
    std::stringstream ss(clipboard);
    std::string line;
    while (std::getline(ss, line)) paste_lines.push_back(line);

    if (paste_lines.empty()) return;

    if (paste_lines.size() == 1) {
        lines[cy].insert(cx, paste_lines[0]);
        cx += static_cast<int>(paste_lines[0].size());
    } else {
        std::string rest = lines[cy].substr(cx);
        lines[cy].erase(cx);
        lines[cy] += paste_lines[0];
        for (size_t i = 1; i < paste_lines.size(); ++i) {
            lines.insert(lines.begin() + cy + static_cast<int>(i), paste_lines[i]);
        }
        int last_idx = cy + static_cast<int>(paste_lines.size()) - 1;
        lines[last_idx] += rest;
        cy = last_idx;
        cx = static_cast<int>(paste_lines.back().size());
    }
    modified = true;
    mark_active = false;
}

void Editor::select_all() {
    mark_active = true;
    mark_cy = 0;
    mark_cx = 0;
    cy = line_count() - 1;
    cx = static_cast<int>(lines.back().size());
}

void Editor::delete_selection() {
    if (!has_selection()) return;
    snapshot();
    auto sel = get_selection();
    int sy = sel[0], sx = sel[1], ey = sel[2], ex = sel[3];
    if (sy == ey) {
        lines[sy].erase(sx, ex - sx);
        cy = sy; cx = sx;
    } else {
        lines[sy].erase(sx);
        lines[sy] += lines[ey].substr(ex);
        for (int i = ey; i > sy; --i)
            lines.erase(lines.begin() + i);
        cy = sy; cx = sx;
    }
    mark_active = false;
    modified = true;
}

// ----------------------------------------------------------------------
// Navigation
// ----------------------------------------------------------------------
bool Editor::go_to_line(int target) {
    if (target < 1 || target > (int)lines.size()) return false;
    cy = target - 1;
    cx = 0;
    ensure_cursor_visible();
    return true;
}

// ----------------------------------------------------------------------
// Bracket matching
// ----------------------------------------------------------------------
bool Editor::cursor_on_bracket() const {
    if (cy >= (int)lines.size() || cx >= (int)lines[cy].size())
        return false;
    char ch = lines[cy][cx];
    static const std::set<char> brackets = {'(',')','[',']','{','}'};
    return brackets.count(ch) > 0;
}

std::pair<int,int> Editor::find_bracket_match() const {
    if (!cursor_on_bracket()) return {-1, -1};
    const auto& l = lines[cy];
    char ch = l[cx];
    static const std::set<char> open_br = {'(','[','{'};
    static const std::set<char> close_br = {')',']','}'};
    static const std::map<char,char> match = {
        {')','('},{']','['},{'}','{'},
        {'(',')'},{'[',']'},{'{','}'}
    };

    if (open_br.count(ch)) {
        char target = match.at(ch);
        int depth = 0;
        for (int row = cy; row < (int)lines.size(); ++row) {
            int start = (row == cy) ? cx + 1 : 0;
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
            int end = (row == cy) ? cx : (int)lines[row].size();
            const auto& line = lines[row];
            for (int col = end - 1; col >= 0; --col) {
                char c = line[col];
                if (c == ch) depth++;
                else if (c == target) {
                    if (depth == 0) return {row, col};
                    depth--;
                }
            }
        }
    }
    return {-1, -1};
}

// ----------------------------------------------------------------------
// Hex mode
// ----------------------------------------------------------------------
std::string Editor::get_hex_line(int line_idx) const {
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

void Editor::toggle_hex_mode() {
    hex_mode = !hex_mode;
    if (hex_mode) {
        // In hex mode, fine cursor navigation on byte boundaries
        sx = 0;
    }
}

// ----------------------------------------------------------------------
// Search highlights
// ----------------------------------------------------------------------
void Editor::clear_search_highlights() {
    search_matches.clear();
    search_match_idx = 0;
}

void Editor::highlight_matches(const std::string& query) {
    search_matches.clear();
    if (query.empty()) return;
    for (int i = 0; i < (int)lines.size(); ++i) {
        auto& line = lines[i];
        size_t pos = 0;
        while ((pos = line.find(query, pos)) != std::string::npos) {
            search_matches.push_back({i, (int)pos});
            pos += query.size();
        }
    }
}

// ----------------------------------------------------------------------
// Indent detection
// ----------------------------------------------------------------------
void Editor::detect_indent() {
    int tabs = 0, spaces = 0;
    std::map<int,int> sizes;
    int scan = std::min(200, (int)lines.size());
    for (int i = 0; i < scan; ++i) {
        const auto& line = lines[i];
        if (line.empty()) continue;
        if (line[0] == '\t') tabs++;
        else if (line[0] == ' ') {
            spaces++;
            int n = (int)line.find_first_not_of(' ');
            if (n > 0) sizes[n]++;
        }
    }
    indent_char = (tabs > spaces) ? '\t' : ' ';
    if (!sizes.empty()) {
        int best = 0, best_count = 0;
        for (auto& p : sizes) {
            if (p.second > best_count) {
                best_count = p.second;
                best = p.first;
            }
        }
        indent_size = best > 0 ? best : 4;
    } else {
        indent_size = 4;
    }
}

// ----------------------------------------------------------------------
// Language detection
// ----------------------------------------------------------------------
std::string Editor::detect_lang(const std::string& path) {
    return get_lang_from_ext(path);
}

std::string Editor::get_lang_from_ext(const std::string& path) {
    static const std::map<std::string,std::string> ext_lang = {
        {".js","js"},{".ts","js"},{".jsx","js"},{".tsx","js"},{".mjs","js"},
        {".py","py"},{".pyi","py"},
        {".json","json"},{".jsonc","json"},
        {".md","md"},{".markdown","md"},
        {".cpp","cpp"},{".c","c"},{".h","c"},{".hpp","cpp"},{".cc","cpp"},{".cxx","cpp"},
        {".rs","rs"},{".go","go"},{".rb","rb"},{".java","java"},
        {".sh","sh"},{".bash","sh"},{".zsh","sh"},
        {".html","html"},{".htm","html"},
        {".css","css"},{".scss","css"},
        {".yaml","yaml"},{".yml","yaml"},
        {".toml","toml"},
        {".txt","text"},
        {".xml","xml"},{".svg","xml"},
        {".sql","sql"},
        {".lua","lua"},
        {".php","php"},
    };
    auto ext = fs::path(path).extension().string();
    auto it = ext_lang.find(ext);
    if (it != ext_lang.end()) return it->second;
    // Also check if filename starts with a known name
    auto fname = fs::path(path).filename().string();
    if (fname == "Makefile" || fname == "makefile") return "makefile";
    if (fname == "Dockerfile") return "dockerfile";
    if (fname == "CMakeLists.txt") return "cmake";
    return "text";
}

// ----------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------
void Editor::clamp_cursor() {
    cy = std::max(0, std::min(line_count() - 1, cy));
    cx = std::max(0, std::min(static_cast<int>(lines[cy].size()), cx));
}

void Editor::ensure_cursor_visible() {
    clamp_cursor();
    if (cy < sy) sy = cy;
    if (cy >= sy + 30) sy = cy - 15;
    if (cx < sx) sx = cx;
    if (cx >= sx + 60) sx = cx - 30;
    sx = std::max(0, sx);
}

void Editor::scroll_to_cursor(int editor_height, int editor_width) {
    int margin = 3;
    int eh = std::max(4, editor_height);
    int ew = std::max(10, editor_width);

    // Vertical scroll
    if (cy < sy) {
        sy = cy;
    } else if (cy >= sy + eh - margin) {
        sy = cy - eh + margin + 1;
    }

    // Horizontal scroll
    if (cx < sx) {
        sx = cx;
    } else if (cx >= sx + ew - margin) {
        sx = cx - ew + margin + 1;
    }

    sy = std::max(0, sy);
    sx = std::max(0, sx);
}
