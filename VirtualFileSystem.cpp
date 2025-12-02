#include "VirtualFileSystem.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>

//VFSNode implementation

VFSNode::VFSNode(const std::string& name, Type type, VFSNode* parent)
    : name(name), type(type), parent(parent), permissions("rwx") {
}

const std::string& VFSNode::getName() const {
    return name;
}

VFSNode::Type VFSNode::getType() const {
    return type;
}

VFSNode* VFSNode::getParent() const {
    return parent;
}

void VFSNode::setPermissions(const std::string& perms) {
    permissions = perms;
}

const std::string& VFSNode::getPermissions() const {
    return permissions;
}

std::string& VFSNode::getContent() {
    return content;
}

const std::string& VFSNode::getContentConst() const {
    return content;
}

bool VFSNode::isDirectory() const {
    return type == Type::Directory;
}

bool VFSNode::isFile() const {
    return type == Type::File;
}

std::vector<std::unique_ptr<VFSNode>>& VFSNode::getChildren() {
    return children;
}

const std::vector<std::unique_ptr<VFSNode>>& VFSNode::getChildrenConst() const {
    return children;
}

VFSNode* VFSNode::findChild(const std::string& name) {
    for (auto& child : children) {
        if (child->getName() == name)
            return child.get();
    }
    return nullptr;
}

const VFSNode* VFSNode::findChildConst(const std::string& name) const {
    for (const auto& child : children) {
        if (child->getName() == name)
            return child.get();
    }
    return nullptr;
}

VFSNode* VFSNode::addDirectory(const std::string& name) {
    children.push_back(std::make_unique<VFSNode>(name, Type::Directory, this));
    return children.back().get();
}

VFSNode* VFSNode::addFile(const std::string& name) {
    children.push_back(std::make_unique<VFSNode>(name, Type::File, this));
    return children.back().get();
}

bool VFSNode::removeChild(const std::string& name) {
    auto it = std::remove_if(children.begin(), children.end(),
        [&](const std::unique_ptr<VFSNode>& n) {
            return n->getName() == name;
        });

    if (it != children.end()) {
        children.erase(it, children.end());
        return true;
    }
    return false;
}

void VFSNode::listChildren(bool showPermissions) const {
    for (const auto& child : children) {
        char typeChar = child->isDirectory() ? 'd' : '-';
        if (showPermissions) {
            std::cout << typeChar << child->getPermissions() << "  " << child->getName() << "\n";
        }
        else {
            std::cout << child->getName() << "\n";
        }
    }
}

//virtualFileSystem impl


VirtualFileSystem::VirtualFileSystem(const std::string& saveFile)
    : saveFileName(saveFile) {
    root = std::make_unique<VFSNode>("/", VFSNode::Type::Directory, nullptr);
    root->setPermissions("rwx");
    current = root.get();
}

// Path utilities

std::vector<std::string> VirtualFileSystem::splitPath(const std::string& path) const {
    std::vector<std::string> result;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (!item.empty() && item != ".")
            result.push_back(item);
    }
    return result;
}

VFSNode* VirtualFileSystem::resolvePath(const std::string& path) {
    if (path.empty()) return current;

    VFSNode* node = (path[0] == '/') ? root.get() : current;
    auto parts = splitPath(path);

    for (const auto& part : parts) {
        if (part == "..") {
            if (node->getParent())
                node = node->getParent();
        }
        else {
            VFSNode* next = node->findChild(part);
            if (!next) return nullptr;
            node = next;
        }
    }
    return node;
}

const VFSNode* VirtualFileSystem::resolvePathConst(const std::string& path) const {
    if (path.empty()) return current;

    const VFSNode* node = (path[0] == '/') ? root.get() : current;
    auto parts = splitPath(path);

    for (const auto& part : parts) {
        if (part == "..") {
            if (node->getParent())
                node = node->getParent();
        }
        else {
            const VFSNode* next = node->findChildConst(part);
            if (!next) return nullptr;
            node = next;
        }
    }
    return node;
}

bool VirtualFileSystem::checkPermission(const VFSNode* node, char need) const {
    return node->getPermissions().find(need) != std::string::npos;
}

std::string VirtualFileSystem::getCurrentPath() const {
    std::vector<std::string> parts;
    const VFSNode* node = current;

    while (node && node != root.get()) {
        parts.push_back(node->getName());
        node = node->getParent();
    }

    std::string result = "/";
    for (int i = parts.size() - 1; i >= 0; --i) {
        result += parts[i];
        if (i != 0) result += "/";
    }

    return result;
}

// basic commands

void VirtualFileSystem::cmdPwd() const {
    std::cout << getCurrentPath() << "\n";
}

void VirtualFileSystem::cmdLs() const {
    if (!checkPermission(current, 'r')) {
        std::cout << "Permission denied.\n";
        return;
    }
    current->listChildren(true);
}

bool VirtualFileSystem::cmdCd(const std::string& path) {
    if (path.empty()) {
        current = root.get();
        return true;
    }

    VFSNode* target = resolvePath(path);
    if (!target) {
        std::cout << "cd: no such directory\n";
        return false;
    }
    if (!target->isDirectory()) {
        std::cout << "cd: not a directory\n";
        return false;
    }
    if (!checkPermission(target, 'x')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    current = target;
    return true;
}

bool VirtualFileSystem::cmdMkdir(const std::string& path) {
    if (path.empty()) {
        std::cout << "mkdir: missing operand\n";
        return false;
    }

    size_t slash = path.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "" : path.substr(0, slash);
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);

    VFSNode* parent = parentPath.empty() ? current : resolvePath(parentPath);
    if (!parent || !parent->isDirectory()) {
        std::cout << "mkdir: invalid parent directory\n";
        return false;
    }

    if (parent->findChild(name)) {
        std::cout << "mkdir: already exists\n";
        return false;
    }

    if (!checkPermission(parent, 'w')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    VFSNode* dir = parent->addDirectory(name);
    dir->setPermissions("rwx");
    return true;
}

bool VirtualFileSystem::cmdTouch(const std::string& path) {
    if (path.empty()) {
        std::cout << "touch: missing operand\n";
        return false;
    }

    size_t slash = path.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "" : path.substr(0, slash);
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);

    VFSNode* parent = parentPath.empty() ? current : resolvePath(parentPath);
    if (!parent || !parent->isDirectory()) {
        std::cout << "touch: invalid directory\n";
        return false;
    }

    if (parent->findChild(name))
        return true; 

    if (!checkPermission(parent, 'w')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    VFSNode* file = parent->addFile(name);
    file->setPermissions("rw-");
    return true;
}

bool VirtualFileSystem::cmdRm(const std::string& path, bool recursive) {
    if (path.empty()) {
        std::cout << "rm: missing operand\n";
        return false;
    }

    if (path == "/") {
        std::cout << "rm: cannot remove root\n";
        return false;
    }

    size_t slash = path.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "" : path.substr(0, slash);
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);

    VFSNode* parent = parentPath.empty() ? current : resolvePath(parentPath);
    if (!parent || !parent->isDirectory()) {
        std::cout << "rm: invalid parent\n";
        return false;
    }

    VFSNode* target = parent->findChild(name);
    if (!target) {
        std::cout << "rm: not found\n";
        return false;
    }

    if (!checkPermission(parent, 'w')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    if (target->isDirectory() && !recursive && !target->getChildrenConst().empty()) {
        std::cout << "rm: directory not empty (use -r)\n";
        return false;
    }

    parent->removeChild(name);
    return true;
}

// file viweing / editing

bool VirtualFileSystem::cmdCat(const std::string& path) const {
    const VFSNode* node = resolvePathConst(path);
    if (!node || !node->isFile()) {
        std::cout << "cat: invalid file\n";
        return false;
    }

    if (!checkPermission(node, 'r')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    std::cout << node->getContentConst() << "\n";
    return true;
}

bool VirtualFileSystem::cmdWrite(const std::string& path) {
    VFSNode* node = resolvePath(path);

    if (!node) {
        if (!cmdTouch(path)) return false;
        node = resolvePath(path);
    }

    if (!node->isFile()) {
        std::cout << "write: not a file\n";
        return false;
    }

    if (!checkPermission(node, 'w')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    std::cout << "Enter text. End with .end\n";
    std::string line;
    std::ostringstream buffer;

    while (true) {
        std::getline(std::cin, line);
        if (line == ".end") break;
        buffer << line << "\n";
    }

    node->getContent() = buffer.str();
    return true;
}

//copy/move

void VirtualFileSystem::copyNodeRecursive(const VFSNode* src, VFSNode* dst, const std::string& newName) {
    VFSNode* newNode = nullptr;

    if (src->isDirectory()) {
        newNode = dst->addDirectory(newName);
    }
    else {
        newNode = dst->addFile(newName);
        newNode->getContent() = src->getContentConst();
    }

    newNode->setPermissions(src->getPermissions());

    if (src->isDirectory()) {
        for (const auto& child : src->getChildrenConst()) {
            copyNodeRecursive(child.get(), newNode, child->getName());
        }
    }
}

bool VirtualFileSystem::cmdCp(const std::string& srcPath, const std::string& dstPath) {
    const VFSNode* src = resolvePathConst(srcPath);
    if (!src) {
        std::cout << "cp: invalid source\n";
        return false;
    }

    if (!checkPermission(src, 'r')) {
        std::cout << "Permission denied.\n";
        return false;
    }

    size_t slash = dstPath.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "" : dstPath.substr(0, slash);
    std::string name = (slash == std::string::npos) ? dstPath : dstPath.substr(slash + 1);

    VFSNode* parent = parentPath.empty() ? current : resolvePath(parentPath);
    if (!parent || !parent->isDirectory()) {
        std::cout << "cp: invalid target\n";
        return false;
    }

    if (parent->findChild(name)) {
        std::cout << "cp: target exists\n";
        return false;
    }

    copyNodeRecursive(src, parent, name);
    return true;
}

bool VirtualFileSystem::cmdMv(const std::string& srcPath, const std::string& dstPath) {
    const VFSNode* srcConst = resolvePathConst(srcPath);
    if (!srcConst) {
        std::cout << "mv: invalid source\n";
        return false;
    }

    VFSNode* src = resolvePath(srcPath);
    VFSNode* oldParent = src->getParent();

    if (!oldParent) {
        std::cout << "mv: cannot move root\n";
        return false;
    }

    size_t slash = dstPath.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "" : dstPath.substr(0, slash);
    std::string name = (slash == std::string::npos) ? dstPath : dstPath.substr(slash + 1);

    VFSNode* parent = parentPath.empty() ? current : resolvePath(parentPath);
    if (!parent || !parent->isDirectory()) {
        std::cout << "mv: invalid target\n";
        return false;
    }

    if (parent->findChild(name)) {
        std::cout << "mv: target exists\n";
        return false;
    }

    copyNodeRecursive(src, parent, name);
    oldParent->removeChild(src->getName());

    return true;
}

//chmod

bool VirtualFileSystem::cmdChmod(const std::string& perms, const std::string& path) {
    if (perms.size() != 3) {
        std::cout << "chmod: invalid permissions\n";
        return false;
    }

    VFSNode* n = resolvePath(path);
    if (!n) {
        std::cout << "chmod: no such file\n";
        return false;
    }

    n->setPermissions(perms);
    return true;
}

// tree printnter

void VirtualFileSystem::cmdTree() const {
    struct Helper {
        static void print(const VFSNode* node, const std::string& prefix, bool last, const VFSNode* root) {
            std::cout << prefix;

            if (node != root)
                std::cout << (last ? "`-- " : "|-- ");

            if (node == root)
                std::cout << "/\n";
            else
                std::cout << node->getName() << "\n";

            const auto& kids = node->getChildrenConst();
            for (size_t i = 0; i < kids.size(); ++i) {
                bool childLast = (i == kids.size() - 1);
                std::string newPrefix = prefix;
                if (node != root)
                    newPrefix += (last ? "    " : "|   ");

                print(kids[i].get(), newPrefix, childLast, root);
            }
        }
    };

    Helper::print(root.get(), "", true, root.get());
}

//save/load

void VirtualFileSystem::saveNodeRecursive(const VFSNode* node, const std::string& path, std::ostream& out) const {
    std::string fullPath = path;

    if (node != root.get()) {
        if (fullPath != "/")
            fullPath += "/";
        fullPath += node->getName();
    }

    out << "NODE " << (node->isDirectory() ? "DIR " : "FILE ")
        << node->getPermissions() << " " << fullPath << "\n";

    if (node->isFile()) {
        out << "CONTENT_BEGIN\n";
        out << node->getContentConst();
        out << "CONTENT_END\n";
    }

    for (const auto& child : node->getChildrenConst()) {
        saveNodeRecursive(child.get(), fullPath, out);
    }
}

void VirtualFileSystem::save() const {
    std::ofstream out(saveFileName);
    if (!out) {
        std::cout << "Could not save filesystem.\n";
        return;
    }
    saveNodeRecursive(root.get(), "/", out);
}

void VirtualFileSystem::buildPathDirectory(const std::string& dirPath) {
    ensureDirectory(dirPath);
}

VFSNode* VirtualFileSystem::ensureDirectory(const std::string& dirPath) {
    if (dirPath.empty() || dirPath == "/")
        return root.get();

    VFSNode* node = root.get();
    auto parts = splitPath(dirPath);

    for (const auto& part : parts) {
        VFSNode* child = node->findChild(part);
        if (!child) {
            child = node->addDirectory(part);
            child->setPermissions("rwx");
        }
        node = child;
    }

    return node;
}

VFSNode* VirtualFileSystem::createFileAtPath(const std::string& filePath) {
    size_t slash = filePath.find_last_of('/');
    std::string parentPath = (slash == std::string::npos) ? "/" : filePath.substr(0, slash);
    std::string filename = (slash == std::string::npos) ? filePath : filePath.substr(slash + 1);

    VFSNode* parent = ensureDirectory(parentPath);
    VFSNode* file = parent->findChild(filename);

    if (!file) {
        file = parent->addFile(filename);
        file->setPermissions("rw-");
    }

    return file;
}

void VirtualFileSystem::load() {
    std::ifstream in(saveFileName);
    if (!in) {
        VFSNode* home = root->addDirectory("home");
        home->setPermissions("rwx");

        VFSNode* docs = home->addDirectory("docs");
        docs->setPermissions("rwx");

        VFSNode* readme = docs->addFile("readme.txt");
        readme->setPermissions("rw-");
        readme->getContent() =
            "Welcome to the Virtual File System Shell.\n"
            "Use commands like ls, cd, mkdir, touch, cat, write, rm, cp, mv.\n";

        current = root.get();
        return;
    }

    //reset/ rebuild tree
    root = std::make_unique<VFSNode>("/", VFSNode::Type::Directory, nullptr);
    root->setPermissions("rwx");
    current = root.get();

    std::string line;
    bool readingContent = false;
    VFSNode* lastFile = nullptr;
    std::ostringstream buffer;

    while (std::getline(in, line)) {
        if (line.rfind("NODE ", 0) == 0) {
            if (readingContent && lastFile) {
                lastFile->getContent() = buffer.str();
                buffer.str("");
                buffer.clear();
                readingContent = false;
            }

            std::stringstream ss(line);
            std::string token, type, perms, path;
            ss >> token >> type >> perms >> path;

            if (type == "DIR") {
                VFSNode* d = ensureDirectory(path);
                d->setPermissions(perms);
            }
            else if (type == "FILE") {
                lastFile = createFileAtPath(path);
                lastFile->setPermissions(perms);
            }
        }
        else if (line == "CONTENT_BEGIN") {
            readingContent = true;
            buffer.str("");
            buffer.clear();
        }
        else if (line == "CONTENT_END") {
            if (lastFile)
                lastFile->getContent() = buffer.str();

            readingContent = false;
            lastFile = nullptr;
            buffer.str("");
            buffer.clear();
        }
        else if (readingContent) {
            buffer << line << "\n";
        }
    }

    if (readingContent && lastFile)
        lastFile->getContent() = buffer.str();

    current = root.get();
}
