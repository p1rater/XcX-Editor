#pragma once

#include "common.h"
#include <vector>
#include <string>

struct Theme {
    std::string name;
    std::string description;
    ColorDef colors[NUM_COLOR_PAIRS];
};

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager() = default;

    void apply_theme(int index);
    void next_theme();
    void prev_theme();
    int current_index() const { return current_; }
    int count() const { return static_cast<int>(themes_.size()); }
    const Theme& current() const { return themes_[current_]; }
    const Theme& get(int idx) const { return themes_[idx]; }
    std::string current_name() const { return themes_[current_].name; }
    std::vector<std::string> theme_names() const;

private:
    void build_themes();
    std::vector<Theme> themes_;
    int current_ = 0;
};

// Convenience: call this after init_colors in the theme
void init_color_pairs(const Theme& theme);
