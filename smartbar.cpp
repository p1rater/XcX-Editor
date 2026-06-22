#include "smartbar.h"
#include "editor.h"
#include <cctype>
#include <cstring>

// ----------------------------------------------------------------------
// Open / Close
// ----------------------------------------------------------------------
void SmartBar::open(Mode m, const std::string& prefill) {
    mode = m;
    text = prefill;
    replace_text.clear();
    replace_field = 0;
    results.clear();
    idx = 0;
}

void SmartBar::close() {
    mode = NONE;
    text.clear();
    replace_text.clear();
    results.clear();
    idx = 0;
}

// ----------------------------------------------------------------------
// Mode title for UI
// ----------------------------------------------------------------------
std::string SmartBar::mode_title() const {
    switch (mode) {
        case SEARCH:   return " SEARCH ";
        case PATH:     return " GO TO FOLDER ";
        case CMD:      return " COMMAND PALETTE ";
        case OPEN:     return " QUICK OPEN ";
        case GREP:     return " GREP ";
        case REPLACE:  return " FIND & REPLACE ";
        case OUTLINE:  return " OUTLINE ";
        case GOTO:     return " GO TO LINE ";
        default:       return "";
    }
}

// ----------------------------------------------------------------------
// Input
// ----------------------------------------------------------------------
void SmartBar::type(char ch) {
    if (mode == REPLACE && replace_field == 1)
        replace_text.push_back(ch);
    else
        text.push_back(ch);
}

void SmartBar::backspace() {
    if (mode == REPLACE && replace_field == 1) {
        if (!replace_text.empty()) replace_text.pop_back();
    } else {
        if (!text.empty()) text.pop_back();
    }
}

void SmartBar::nav(int d) {
    if (!results.empty())
        idx = (idx + d + (int)results.size()) % (int)results.size();
}

// ----------------------------------------------------------------------
// Update results based on mode
// ----------------------------------------------------------------------
void SmartBar::update(const Editor& ed, const fs::path& root) {
    switch (mode) {
        case SEARCH:  update_search(ed); break;
        case CMD:     update_cmd(); break;
        case OPEN:    update_open(root); break;
        case GREP:    update_grep(root); break;
        case GOTO:    update_goto(ed); break;
        case OUTLINE: update_outline(ed); break;
        case REPLACE: update_replace(ed); break;
        default: break;
    }
    if (idx >= (int)results.size()) idx = std::max(0, (int)results.size() - 1);
    if (idx < 0) idx = 0;
}

// ----------------------------------------------------------------------
// SEARCH — incremental in-file search
// ----------------------------------------------------------------------
void SmartBar::update_search(const Editor& ed) {
    results.clear();
    std::string q = text;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (int i = 0; i < (int)ed.lines.size(); ++i) {
        if (q.empty()) continue;
        std::string low_line = ed.lines[i];
        std::transform(low_line.begin(), low_line.end(), low_line.begin(), ::tolower);
        if (low_line.find(q) != std::string::npos) {
            std::string preview = ed.lines[i].substr(0, 58);
            results.push_back(std::to_string(i + 1) + "  " + preview);
        }
    }
}

// ----------------------------------------------------------------------
// CMD — command palette
// ----------------------------------------------------------------------
void SmartBar::update_cmd() {
    results.clear();
    static const std::vector<std::pair<std::string,std::string>> commands = {
        {"save",       "Write buffer to disk"},
        {"new",        "Create empty buffer"},
        {"help",       "Show keybinding reference"},
        {"outline",    "Jump to symbol"},
        {"zen",        "Toggle zen mode"},
        {"theme next", "Next color theme"},
        {"theme prev", "Previous color theme"},
        {"git status", "Short git status"},
        {"git log",    "Last commits"},
        {"exit",       "Quit XcX editor"},
        {"quit",       "Quit XcX editor"},
        {"hex",        "Toggle hex mode"},
        {"undo",       "Undo last change"},
        {"redo",       "Redo last undone change"},
        {"select all", "Select entire buffer"},
    };

    std::string q = text;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto& cmd : commands) {
        std::string low_cmd = cmd.first;
        std::transform(low_cmd.begin(), low_cmd.end(), low_cmd.begin(), ::tolower);
        if (q.empty() || low_cmd.find(q) != std::string::npos) {
            results.push_back(cmd.first + "  -  " + cmd.second);
        }
    }
}

// ----------------------------------------------------------------------
// OPEN — fuzzy file quick-open
// ----------------------------------------------------------------------
void SmartBar::update_open(const fs::path& root) {
    results.clear();
    std::string q = text;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    try {
        for (auto& p : fs::recursive_directory_iterator(root)) {
            if (!p.is_regular_file()) continue;
            auto name = p.path().filename().string();
            if (name.empty() || name[0] == '.') continue;
            std::string low = name;
            std::transform(low.begin(), low.end(), low.begin(), ::tolower);
            // Fuzzy match: all chars in q must appear in order in name
            bool match = true;
            size_t pos = 0;
            for (char c : q) {
                pos = low.find(c, pos);
                if (pos == std::string::npos) { match = false; break; }
                pos++;
            }
            if (match) {
                results.push_back(p.path().string());
                if (results.size() >= (size_t)MAX_RESULTS) break;
            }
        }
    } catch (...) {}
}

// ----------------------------------------------------------------------
// GREP — global search using grep binary
// ----------------------------------------------------------------------
void SmartBar::update_grep(const fs::path& root) {
    results.clear();
    if (text.empty()) return;

    // Escape double quotes in the query
    std::string escaped = text;
    size_t p = 0;
    while ((p = escaped.find('"', p)) != std::string::npos) {
        escaped.insert(p, "\\");
        p += 2;
    }

    std::string cmd = "grep -rn --include=* --color=never -I \"" +
                      escaped + "\" " + root.string() + " 2>/dev/null | head -100";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return;
    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        if (!line.empty() && line.back() == '\n') line.pop_back();
        // Strip absolute path
        auto pos = line.find(root.string());
        if (pos != std::string::npos)
            line.replace(pos, root.string().size(), "");
        results.push_back(line);
    }
    pclose(pipe);
}

// ----------------------------------------------------------------------
// GOTO — go to line number
// ----------------------------------------------------------------------
void SmartBar::update_goto(const Editor& ed) {
    results.clear();
    if (text.empty()) return;
    try {
        int ln = std::stoi(text);
        if (ln >= 1 && ln <= (int)ed.lines.size()) {
            std::string preview = ed.lines[ln - 1].substr(0, 58);
            results.push_back(std::to_string(ln) + "  " + preview);
        } else {
            results.push_back("Line out of range (1-" + std::to_string(ed.lines.size()) + ")");
        }
    } catch (...) {
        results.push_back("Enter a line number");
    }
}

// ----------------------------------------------------------------------
// OUTLINE — symbol navigation
// ----------------------------------------------------------------------
void SmartBar::update_outline(const Editor& ed) {
    results.clear();
    std::string q = text;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (int i = 0; i < (int)ed.lines.size(); ++i) {
        auto& l = ed.lines[i];
        std::string stripped = l;
        stripped.erase(0, stripped.find_first_not_of(" \t"));
        bool match = false;
        std::string label;

        // Python
        if (stripped.rfind("def ", 0) == 0) {
            auto endpos = stripped.find('(');
            label = stripped.substr(4, endpos - 4);
            match = true;
        } else if (stripped.rfind("class ", 0) == 0) {
            auto endpos = stripped.find(':');
            label = stripped.substr(6, endpos - 6);
            match = true;
        }
        // JS/TS
        else if (stripped.rfind("function ", 0) == 0) {
            auto endpos = stripped.find('(');
            label = stripped.substr(9, endpos - 9);
            match = true;
        } else if (stripped.rfind("const ", 0) == 0 && stripped.find("=>") != std::string::npos) {
            auto eqpos = stripped.find('=');
            label = stripped.substr(6, eqpos - 7);
            match = true;
        }

        if (match) {
            if (!q.empty()) {
                std::string low_label = label;
                std::transform(low_label.begin(), low_label.end(), low_label.begin(), ::tolower);
                if (low_label.find(q) == std::string::npos) continue;
            }
            results.push_back(std::to_string(i + 1) + "  " + label);
        }
    }
}

// ----------------------------------------------------------------------
// REPLACE — find & replace preview
// ----------------------------------------------------------------------
void SmartBar::update_replace(const Editor& ed) {
    results.clear();
    if (text.empty()) return;
    for (int i = 0; i < (int)ed.lines.size(); ++i) {
        if (ed.lines[i].find(text) != std::string::npos) {
            std::string preview = ed.lines[i].substr(0, 56);
            results.push_back(std::to_string(i + 1) + "  " + preview);
            if (results.size() >= 20) break;
        }
    }
}

// ----------------------------------------------------------------------
// Results parsing helpers
// ----------------------------------------------------------------------
int SmartBar::hit_line() const {
    if (results.empty()) return -1;
    if (mode != SEARCH && mode != OUTLINE && mode != REPLACE) return -1;
    try {
        auto space_pos = results[idx].find(' ');
        if (space_pos == std::string::npos) return -1;
        return std::stoi(results[idx].substr(0, space_pos)) - 1;
    } catch (...) { return -1; }
}

std::string SmartBar::hit_cmd() const {
    if (mode == CMD && !results.empty()) {
        auto space_pos = results[idx].find(' ');
        if (space_pos != std::string::npos)
            return results[idx].substr(0, space_pos);
    }
    return text;
}

std::string SmartBar::hit_file() const {
    if (mode == OPEN && !results.empty())
        return results[idx];
    return "";
}

std::pair<std::string,int> SmartBar::hit_grep() const {
    if (mode == GREP && !results.empty()) {
        auto& r = results[idx];
        size_t colon1 = r.find(':');
        if (colon1 != std::string::npos) {
            size_t colon2 = r.find(':', colon1 + 1);
            if (colon2 != std::string::npos) {
                try {
                    std::string f = r.substr(0, colon1);
                    int ln = std::stoi(r.substr(colon1 + 1, colon2 - colon1 - 1));
                    return {f, ln - 1};  // return 0-indexed
                } catch (...) {}
            }
        }
    }
    return {"", -1};
}
