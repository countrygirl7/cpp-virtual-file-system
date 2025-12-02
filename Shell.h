#pragma once

#include "VirtualFileSystem.h"
#include <string>
#include <vector>

class Shell {
private:
    VirtualFileSystem& vfs;
    std::vector<std::string> history;
    bool running;

    void addToHistory(const std::string& line);
    void printPrompt() const;
    void handleCommand(const std::string& line);

public:
    Shell(VirtualFileSystem& vfs);

    void run();
};
