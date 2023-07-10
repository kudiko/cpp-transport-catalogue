#pragma once
#include "json.h"

namespace json
{
class Builder;
class ArrayItemContext;
class DictItemContext;
class DictKeyContext;

class Context
{
    public:
    Context(Builder& builder);

    Builder& builder_;
};

class ArrayItemContext : public Context
{
    public:
    ArrayItemContext(Builder& builder);

    ArrayItemContext Value(Node::Value value);
    ArrayItemContext StartArray();
    DictItemContext StartDict();
    Builder& EndArray();
};

class DictItemContext : public Context
{
    public:
    DictItemContext(Builder& builder);

    DictKeyContext Key(std::string key);
    Builder& EndDict();
};

class DictKeyContext : public Context
{
    public:
    DictKeyContext(Builder& builder);

    DictItemContext Value(Node::Value value);
    ArrayItemContext StartArray();
    DictItemContext StartDict();
};

class Builder
{
    public:
    Builder();

    Node Build();

    DictKeyContext Key(std::string);
    Builder& Value(Node::Value);

    DictItemContext StartDict();
    Builder& EndDict();

    ArrayItemContext StartArray();
    Builder& EndArray();

    private:
    
    Node root_;
    std::vector<Node*> nodes_stack_;

    bool IsReadyCheck();
    void CheckCorrectValuePlacing(const std::string& err_msg);

    void AddValueToParentNode(const Node* node_to_add, Node* parent_node);
};




}