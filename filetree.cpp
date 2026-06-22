#include "filetree.h"
#include <algorithm>

// ----------------------------------------------------------------------
// Construction
// ----------------------------------------------------------------------
FileTree::FileTree(const std::string& r) : root(r) {
    refresh();
}

// ----------------------------------------------------------------------
// Should skip hidden/unwanted directories
// ----------------------------------------------------------------------
bool FileTree::should_skip(const fs::path& p) {
    auto name = p.filename().string();
    if (name.empty()) return false;
    if (name[0] == '.') return true;
    static const std::set<std::string> skip = {
        "node_modules", "__pycache__", ".git", ".svn",
        ".venv", "venv", ".tox", ".mypy_cache", ".pytest_cache",
        "dist", "build", ".idea", ".vscode", ".DS_Store",
        "__MACOSX", "target", ".gradle", "bin", "obj"
    };
    return skip.count(name) > 0;
}

// ----------------------------------------------------------------------
// File icon helper
// ----------------------------------------------------------------------
std::string FileTree::file_icon(const fs::path& p) {
    if (fs::is_directory(p)) return "";
    auto ext = p.extension().string();
    static const std::map<std::string,std::string> icons = {
        {".py", "PY"}, {".js", "JS"}, {".ts", "TS"}, {".jsx", "RX"},
        {".tsx", "TX"}, {".cpp", "C+"}, {".c", "C "}, {".h", "H "},
        {".hpp", "H+"}, {".rs", "RS"}, {".go", "GO"}, {".rb", "RB"},
        {".java", "JV"}, {".json", "{}"}, {".md", "MD"}, {".css", "CS"},
        {".scss", "SC"}, {".html", "HT"}, {".htm", "HT"}, {".sh", "SH"},
        {".bash", "SH"}, {".zsh", "SH"}, {".txt", "TX"}, {".yml", "YM"},
        {".yaml", "YM"}, {".toml", "TM"}, {".env", "EN"}, {".lock", "LK"},
        {".xml", "XM"}, {".svg", "SV"}, {".sql", "SQ"}, {".lua", "LU"},
        {".php", "PH"}, {".swift", "SW"}, {".kt", "KT"}, {".dart", "DA"},
        {".ex", "EX"}, {".exs", "EX"}, {".zig", "ZG"},
        {".Makefile", "MK"}, {".cmake", "CM"},
    };
    auto it = icons.find(ext);
    if (it != icons.end()) return it->second;
    // Check full filename
    auto fname = p.filename().string();
    static const std::map<std::string,std::string> name_icons = {
        {"Makefile", "MK"}, {"makefile", "MK"}, {"Dockerfile", "DK"},
        {"CMakeLists.txt", "CM"}, {"LICENSE", "LI"}, {"README.md", "RD"},
    };
    auto nit = name_icons.find(fname);
    if (nit != name_icons.end()) return nit->second;
    return "  ";
}

// ----------------------------------------------------------------------
// Walk directory tree
// ----------------------------------------------------------------------
void FileTree::walk(const fs::path& dir, int depth) {
    try {
        std::vector<fs::directory_entry> entries;
        for (auto& e : fs::directory_iterator(dir))
            entries.push_back(e);

        std::sort(entries.begin(), entries.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
                bool ad = a.is_directory(), bd = b.is_directory();
                if (ad != bd) return ad > bd;
                return a.path().filename() < b.path().filename();
            });

        for (auto& e : entries) {
            auto path = e.path();
            if (should_skip(path)) continue;
            bool is_dir = fs::is_directory(path);
            std::string key = path.string();
            bool expanded = (open_set_.count(key) > 0);
            items.push_back({path, is_dir, expanded, depth});
            if (is_dir && expanded)
                walk(path, depth + 1);
        }
    } catch (...) {
        // Permission errors, etc. — skip silently
    }
}

// ----------------------------------------------------------------------
// Refresh the tree
// ----------------------------------------------------------------------
void FileTree::refresh() {
    items.clear();
    walk(root, 0);
    selected = std::min(selected, std::max(0, (int)items.size() - 1));
    if (scroll > selected) scroll = selected;
}

// ----------------------------------------------------------------------
// Toggle expand/collapse
// ----------------------------------------------------------------------
void FileTree::toggle() {
    if (items.empty()) return;
    auto& item = items[selected];
    if (!item.is_dir) return;
    std::string key = item.path.string();
    if (open_set_.count(key)) {
        open_set_.erase(key);
    } else {
        open_set_.insert(key);
    }
    refresh();
}

// ----------------------------------------------------------------------
// Navigate up to parent directory
// ----------------------------------------------------------------------
void FileTree::go_up() {
    auto parent = root.parent_path();
    if (parent == root || parent.empty()) return;
    root = parent;
    selected = scroll = 0;
    open_set_.clear();
    refresh();
}

// ----------------------------------------------------------------------
// Navigate into a directory
// ----------------------------------------------------------------------
void FileTree::go_into() {
    auto* item = current();
    if (!item || !item->is_dir) return;
    root = item->path;
    selected = scroll = 0;
    open_set_.clear();
    refresh();
}

// ----------------------------------------------------------------------
// Move selection
// ----------------------------------------------------------------------
void FileTree::move(int d) {
    if (items.empty()) return;
    selected = std::max(0, std::min((int)items.size() - 1, selected + d));
    // Auto-scroll
    if (selected < scroll) scroll = selected;
    if (selected >= scroll + 30) scroll = selected - 15;
}

// ----------------------------------------------------------------------
// Get current item
// ----------------------------------------------------------------------
FileTree::Item* FileTree::current() {
    if (items.empty()) return nullptr;
    if (selected >= (int)items.size()) selected = (int)items.size() - 1;
    return &items[selected];
}

const FileTree::Item* FileTree::current() const {
    if (items.empty()) return nullptr;
    if (selected >= (int)items.size()) return nullptr;
    return &items[selected];
}
