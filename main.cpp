#include <iostream>
#include "VirtualFileSystem.h"
#include "Shell.h"

int main() {
    std::cout << "=====================================\n";
    std::cout << "  Virtual File System Shell (vsh)\n";
    std::cout << "  Simulated mini Linux terminal\n";
    std::cout << "=====================================\n";
    std::cout << "Save file: vfs.txt\n";
    std::cout << "Type 'help' to see available commands.\n\n";

    VirtualFileSystem vfs("vfs.txt");
    vfs.load();

    Shell shell(vfs);
    shell.run();

    return 0;
}
