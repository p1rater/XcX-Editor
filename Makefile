# ======================================================================
# XcX Editor v2.0 — Makefile
# ======================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
LDFLAGS  := -lncurses -lstdc++fs

SRCDIR   := .
OBJDIR   := obj
TARGET   := xcx

SOURCES  := main.cpp editor.cpp filetree.cpp smartbar.cpp app.cpp theme.cpp
OBJECTS  := $(SOURCES:%.cpp=$(OBJDIR)/%.o)

.PHONY: all clean rebuild run

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "=== XcX Editor v2.0 — Build complete ==="
	@echo "   Run: ./$(TARGET) [file-or-directory]"

clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "Cleaned."

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG -O0
debug: $(TARGET)
	@echo "=== Debug build ==="

# Install (copy to ~/bin)
install: $(TARGET)
	mkdir -p $(HOME)/bin
	cp $(TARGET) $(HOME)/bin/
	@echo "Installed to $(HOME)/bin/$(TARGET)"

# File listing
files:
	@echo "Source files:"
	@ls -lh *.cpp *.h Makefile 2>/dev/null
	@echo "---"
	@wc -l *.cpp *.h Makefile 2>/dev/null
