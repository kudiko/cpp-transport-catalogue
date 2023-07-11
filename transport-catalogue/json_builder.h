#pragma once
#include "json.h"

namespace json
{

    class Builder
    {
    private:
        class ArrayItemContext;
        class DictItemContext;
        class DictKeyContext;

        class Context
        {
        public:
            Context(Builder &builder);

            Builder &builder_;

            ArrayItemContext Value(Node::Value value);
            DictItemContext ValueInDictItem(Node::Value value);
            ArrayItemContext StartArray();
            Builder &EndArray();
            DictItemContext StartDict();
            DictKeyContext Key(std::string key);
            Builder &EndDict();
        };


        class ArrayItemContext : public Context
        {
        public:
            ArrayItemContext(Builder &builder);

            DictItemContext Value(Node::Value value) = delete;
            DictKeyContext Key(std::string key) = delete;
            Builder &EndDict() = delete;
        };

        class DictItemContext : public Context
        {
        public:
            DictItemContext(Builder &builder);

            ArrayItemContext Value(Node::Value value) = delete;
            DictItemContext ValueInDictItem(Node::Value value) = delete;
            ArrayItemContext StartArray() = delete;
            Builder &EndArray() = delete;
            DictItemContext StartDict() = delete;
        };

        class DictKeyContext : public Context
        {
        public:
            DictKeyContext(Builder &builder);

            ArrayItemContext Value(Node::Value value) = delete;
            Builder &EndArray()  = delete;
            DictKeyContext Key(std::string key) = delete;
            Builder &EndDict() = delete;
        };

    public:
        Builder();

        Node Build();

        DictKeyContext Key(std::string);
        Builder &Value(Node::Value);

        DictItemContext StartDict();
        Builder &EndDict();

        ArrayItemContext StartArray();
        Builder &EndArray();

    private:
        Node root_;
        std::vector<Node *> nodes_stack_;

        bool IsReadyCheck();
        void CheckCorrectValuePlacing(const std::string &err_msg);

        void AddValueToParentNode(const Node *node_to_add, Node *parent_node);
    };

}