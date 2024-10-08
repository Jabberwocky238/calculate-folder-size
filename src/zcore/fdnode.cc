#include "fdnode.h"
#include "zutils.h"
#include <unordered_set>
#include <memory>
#include <iostream>

ZFdNode::ZFdNode(const ZIdentifier &id, ZParent parent): parent(parent), id(id)
{
#ifdef ZQ_DEBUG
    std::cout << "[DEBUG] Creating node: " << id << std::endl;
#endif
    this->size = 0;
    this->children_count = 0;
    this->last_modified = 0;
}
ZFdNode::ZFdNode(const ZIdentifier &id): ZFdNode(id, std::nullopt) {}

ZFdNode::~ZFdNode()
{
    for (auto &child : children)
    {
        child.reset();
    }
}

std::shared_ptr<ZFdNode> ZFdNode::locate(const ZIdentifier &id)
{
    for (auto &child : children)
    {
        if (child->id == id)
        {
            return child;
        }
    }
    auto p = std::optional<std::weak_ptr<ZFdNode>>(weak_from_this());
    std::shared_ptr<ZFdNode> node = std::make_shared<ZFdNode>(id, p);
    children.push_back(node);
    return node;
}
std::string ZFdNode::print()
{
    std::string builder = "\n";
    builder += "Name: " + id + "\n";
    builder += "Path: " + path().string() + "\n";
    builder += "Size: " + std::to_string(float(size / 1024) / 1024) + "MB\n";
    builder += "Children Count: " + std::to_string(children_count) + "\n";
    builder += "Last Modified: " + std::string(std::ctime(&last_modified)) + "\n";
    return builder;
}

std::filesystem::path ZFdNode::path()
{
    std::string builder = id;
    auto optianl_wp = this->parent;
    while (optianl_wp.has_value())
    {
        // not root
        auto wp = optianl_wp.value().lock();
        builder = wp.get()->id + "/" + builder;
        optianl_wp = wp.get()->parent;
    }
    return builder;
}

void ZFdNode::scan()
{
    if (!this->verify())
    {
        this->size = 0;
        this->children_count = 0;
        this->last_modified = zutils::getLastModified(this->path());
        
        std::unordered_set<std::string> _children;
        for (const auto &entry : std::filesystem::directory_iterator(this->path()))
        {
            if (entry.is_directory())
            {
                std::shared_ptr<ZFdNode> info = this->locate(entry.path().filename().string());
                info.get()->scan();
                this->size += info.get()->size;
                this->children_count += info.get()->children_count;
                _children.insert(info.get()->id);
            }
            else if (entry.is_regular_file())
            {
                this->size += std::filesystem::file_size(entry.path());
            }
            this->children_count++;
        }
        for (auto it = this->children.begin(); it != this->children.end(); it++)
        {
            bool found = _children.find((*it)->id) != _children.end();
            if (!found)
            {
                this->children.erase(it);
            }
        }
    }
}

bool ZFdNode::verify()
{
    return zutils::getLastModified(this->path()) == this->last_modified;
}
