#include "Shell.h"

#include <iostream>
#include <sstream>
#include <algorithm>

Shell::Shell(VirtualFileSystem& vfsRef)
    : vfs(vfsRef), running(true) {
}

void Shell::addToHistory(const std::string& line) {
    if (!line.empty()) {
        history.push_back(line);
    }
}

void Shell::printPrompt() const {
    std::cout << "vsh:" << vfs.getCurrentPath() << "$ ";
}

void Shell::handleCommand(const std::string& line) {
    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if (cmd.empty()) {
        return;
    }

    if (cmd == "exit" || cmd == "quit") {
        running = false;
        vfs.save();
        std::cout << "Exiting shell. Virtual file system saved.\n";
        return;
    }

    if (cmd == "help") {
        std::cout << "Available commands:\n";
        std::cout << "  pwd                 - print current path\n";
        std::cout << "  ls                  - list directory contents\n";
        std::cout << "  cd <path>           - change directory\n";
        std::cout << "  mkdir <path>        - create directory\n";
        std::cout << "  touch <path>        - create file\n";
        std::cout << "  cat <path>          - show file contents\n";
        std::cout << "  write <path>        - edit file (type .end to finish)\n";
        std::cout << "  rm <path>           - remove file or empty directory\n";
        std::cout << "  rm -r <path>        - remove directory tree\n";
        std::cout << "  cp <src> <dst>      - copy file or directory\n";
        std::cout << "  mv <src> <dst>      - move or rename\n";
        std::cout << "  chmod <perms> <p>   - set permissions (e.g. rw-, r--, rwx)\n";
        std::cout << "  tree                - show directory tree\n";
        std::cout << "  history             - show typed commands\n";
        std::cout << "  save                - save virtual file system to disk\n";
        std::cout << "  help                - show this help\n";
        std::cout << "  exit / quit         - leave shell\n";
        return;
    }

    if (cmd == "pwd") {
        vfs.cmdPwd();
        return;
    }

    if (cmd == "ls") {
        vfs.cmdLs();
        return;
    }

    if (cmd == "cd") {
        std::string path;
        ss >> path;
        vfs.cmdCd(path);
        return;
    }

    if (cmd == "mkdir") {
        std::string path;
        ss >> path;
        vfs.cmdMkdir(path);
        return;
    }

    if (cmd == "touch") {
        std::string path;
        ss >> path;
        vfs.cmdTouch(path);
        return;
    }

    if (cmd == "cat") {
        std::string path;
        ss >> path;
        vfs.cmdCat(path);
        return;
    }

    if (cmd == "write") {
        std::string path;
        ss >> path;
        vfs.cmdWrite(path);
        return;
    }

    if (cmd == "rm") {
        std::string firstArg;
        std::string path;
        ss >> firstArg;
        bool recursive = false;

        if (firstArg == "-r") {
            recursive = true;
            ss >> path;
        }
        else {
            path = firstArg;
        }

        vfs.cmdRm(path, recursive);
        return;
    }

    if (cmd == "cp") {
        std::string src, dst;
        ss >> src >> dst;
        vfs.cmdCp(src, dst);
        return;
    }

    if (cmd == "mv") {
        std::string src, dst;
        ss >> src >> dst;
        vfs.cmdMv(src, dst);
        return;
    }

    if (cmd == "chmod") {
        std::string perms;
        std::string path;
        ss >> perms >> path;
        vfs.cmdChmod(perms, path);
        return;
    }

    if (cmd == "tree") {
        vfs.cmdTree();
        return;
    }

    if (cmd == "save") {
        vfs.save();
        std::cout << "File system saved.\n";
        return;
    }

    if (cmd == "history") {
        for (size_t i = 0; i < history.size(); ++i) {
            std::cout << i + 1 << "  " << history[i] << "\n";
        }
        return;
    }

    std::cout << "Unknown command: " << cmd << "\n";
    std::cout << "Type 'help' to see available commands.\n";
}

void Shell::run() {
    while (running) {
        printPrompt();
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        // Trim spaces
        line.erase(line.begin(),
            std::find_if(line.begin(), line.end(),
                [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(),
            line.end());

        if (line.empty()) {
            continue;
        }

        addToHistory(line);
        handleCommand(line);
    }
}
