#pragma once

#include <string>
#include <vector>
#include <memory>

class VFSNode {
public:
    enum class Type {
        Directory,
        File
    };

private:
    std::string name;
    Type type;
    VFSNode* parent;
    std::vector<std::unique_ptr<VFSNode>> children;
    std::string content;
    std::string permissions;

public:
    VFSNode(const std::string& name, Type type, VFSNode* parent);

    const std::string& getName() const;
    Type getType() const;
    VFSNode* getParent() const;

    void setPermissions(const std::string& perms);
    const std::string& getPermissions() const;

    std::string& getContent();
    const std::string& getContentConst() const;

    bool isDirectory() const;
    bool isFile() const;

    std::vector<std::unique_ptr<VFSNode>>& getChildren();
    const std::vector<std::unique_ptr<VFSNode>>& getChildrenConst() const;

    VFSNode* findChild(const std::string& name);
    const VFSNode* findChildConst(const std::string& name) const;

    VFSNode* addDirectory(const std::string& name);
    VFSNode* addFile(const std::string& name);

    bool removeChild(const std::string& name);

    //debug helper
    void listChildren(bool showPermissions) const;
};

class VirtualFileSystem {
private:
    std::unique_ptr<VFSNode> root;
    VFSNode* current;
    std::string saveFileName;

    //internalhelpers
    std::vector<std::string> splitPath(const std::string& path) const;
    VFSNode* resolvePath(const std::string& path);
    const VFSNode* resolvePathConst(const std::string& path) const;

    bool checkPermission(const VFSNode* node, char needed) const;

    void saveNodeRecursive(const VFSNode* node,
        const std::string& currentPath,
        std::ostream& out) const;

    void buildPathDirectory(const std::string& dirPath);
    VFSNode* ensureDirectory(const std::string& dirPath);
    VFSNode* createFileAtPath(const std::string& filePath);

    void copyNodeRecursive(const VFSNode* src, VFSNode* dstParent, const std::string& newName);

public:
    VirtualFileSystem(const std::string& saveFile);

    void load();
    void save() const;

    std::string getCurrentPath() const;

    //commands
    void cmdPwd() const;
    void cmdLs() const;
    bool cmdCd(const std::string& path);
    bool cmdMkdir(const std::string& path);
    bool cmdTouch(const std::string& path);
    bool cmdRm(const std::string& path, bool recursive);
    bool cmdCat(const std::string& path) const;
    bool cmdWrite(const std::string& path);
    bool cmdCp(const std::string& srcPath, const std::string& dstPath);
    bool cmdMv(const std::string& srcPath, const std::string& dstPath);
    bool cmdChmod(const std::string& perms, const std::string& path);

    void cmdTree() const;
};
