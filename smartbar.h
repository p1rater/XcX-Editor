#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <utility>

class SmartBar {
public:
    enum Mode {
        NONE,
        SEARCH,
        PATH,
        CMD,
        OPEN,
        GREP,
        REPLACE,
        OUTLINE,
        GOTO
    };

    Mode mode = NONE;
    std::string text;
    std::string replace_text;
    int replace_field = 0;    // 0 = find, 1 = replace
    std::vector<std::string> results;
    int idx = 0;

    SmartBar() = default;

    void open(Mode m, const std::string& prefill = "");
    void close();
    bool active() const { return mode != NONE; }
    std::string mode_title() const;

    // Input
    void type(char ch);
    void backspace();
    void nav(int d);
    void update(const Editor& ed, const fs::path& root);

    // Results parsing
    int hit_line() const;
    std::string hit_cmd() const;
    std::string hit_file() const;
    std::pair<std::string,int> hit_grep() const;

private:
    void update_search(const Editor& ed);
    void update_cmd();
    void update_open(const fs::path& root);
    void update_grep(const fs::path& root);
    void update_goto(const Editor& ed);
    void update_outline(const Editor& ed);
    void update_replace(const Editor& ed);
};
