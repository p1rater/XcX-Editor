#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <set>

class FileTree {
public:
    struct Item {
        fs::path path;
        bool is_dir = false;
        bool expanded = false;
        int depth = 0;
    };

    fs::path root;
    std::vector<Item> items;
    int selected = 0;
    int scroll = 0;

    FileTree(const std::string& r = ".");

    void refresh();
    void toggle();
    void go_up();
    void go_into();
    void move(int d);
    Item* current();
    const Item* current() const;
    bool empty() const { return items.empty(); }
    int visible_count() const { return static_cast<int>(items.size()); }

private:
    std::set<std::string> open_set_;
    void walk(const fs::path& dir, int depth);
    static bool should_skip(const fs::path& p);
    static std::string file_icon(const fs::path& p);
};
