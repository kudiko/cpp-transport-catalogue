#include "json_builder.h"

#include <string>
#include <map>

namespace json
{
    Builder::Context::Context(Builder &builder) : builder_{builder} {}

    Builder::ArrayItemContext Builder::Context::Value(Node::Value value)
    {
        return {builder_.Value(value)};
    }
    Builder::ArrayItemContext Builder::Context::StartArray()
    {
        return {builder_.StartArray()};
    }
    Builder::DictItemContext Builder::Context::StartDict()
    {
        return {builder_.StartDict()};
    }
    Builder &Builder::Context::EndArray()
    {
        builder_.EndArray();
        return builder_;
    }

    Builder::DictKeyContext Builder::Context::Key(std::string key)
    {
        return builder_.Key(std::move(key));
    }
    Builder &Builder::Context::EndDict()
    {
        builder_.EndDict();
        return builder_;
    }

    Builder::DictItemContext Builder::Context::ValueInDictItem(Node::Value value)
    {
        return {builder_.Value(value)};
    }

    Builder::ArrayItemContext::ArrayItemContext(Builder &builder) : Context{builder} {}
    Builder::DictItemContext::DictItemContext(Builder &builder) : Context{builder} {}
    Builder::DictKeyContext::DictKeyContext(Builder &builder) : Context{builder} {}

    Builder::Builder()
    {
        nodes_stack_.push_back(&root_);
    }

    Node Builder::Build()
    {
        if (!(nodes_stack_.back() == &root_) || (root_.IsNull()))
        {
            throw std::logic_error("Can't build: context not complete or file is empty");
        }
        return root_;
    }

    Builder::DictItemContext Builder::StartDict()
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call StartDict method for complete object");
        }
        CheckCorrectValuePlacing("StartDict command not after key or not inside in array or not after the constructor");
        Dict new_map;
        Node *dict_ptr = new Node(new_map);
        nodes_stack_.push_back(dict_ptr);

        return {*this};
    }

    Builder &Builder::EndDict()
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call EndDict method for complete object");
        }
        if (!nodes_stack_.back()->IsDict())
        {
            throw std::logic_error("EndDict command in a wrong place");
        }
        Node *complete_dict_node = nodes_stack_.back();
        nodes_stack_.pop_back();
        Value(complete_dict_node->AsDict());

        return *this;
    }

    Builder::ArrayItemContext Builder::StartArray()
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call StartArray method for complete object");
        }
        CheckCorrectValuePlacing("StartArray command not after key or not inside in array or not after the constructor");
        Array new_array;
        Node *arr_ptr = new Node(new_array);
        nodes_stack_.push_back(arr_ptr);

        return {*this};
    }

    Builder &Builder::EndArray()
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call EndArray method for complete object");
        }
        if (!nodes_stack_.back()->IsArray())
        {
            throw std::logic_error("EndArray command in a wrong place");
        }
        Node *complete_arr_node = nodes_stack_.back();
        nodes_stack_.pop_back();
        Value(complete_arr_node->AsArray());

        return *this;
    }

    Builder::DictKeyContext Builder::Key(std::string key)
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call Key method for complete object");
        }
        if (!nodes_stack_.back()->IsDict())
        {
            throw std::logic_error("Key command not inside dict or after another key command");
        }
        Node *key_ptr = new Node(key);
        nodes_stack_.push_back(key_ptr);

        return {*this};
    }

    Builder &Builder::Value(Node::Value value)
    {
        if (IsReadyCheck())
        {
            throw std::logic_error("Trying to call Value method for complete object");
        }
        CheckCorrectValuePlacing("Value command not after key or not inside in array or not after the constructor");
        Node *stack_frame = nodes_stack_.back();
        if (stack_frame->IsString())
        {
            std::string key = stack_frame->AsString();
            nodes_stack_.pop_back();
            Dict &dict = std::get<Dict>(nodes_stack_.back()->GetValue());
            dict[key] = Node(value);
        }
        else if (stack_frame->IsArray())
        {
            Array &array = std::get<Array>(stack_frame->GetValue());
            array.push_back(Node(value));
        }
        else if (stack_frame == &root_)
        {
            root_ = Node(value);
        }

        return *this;
    }

    void Builder::CheckCorrectValuePlacing(const std::string &err_msg)
    {
        Node *stack_frame = nodes_stack_.back();
        if (!(stack_frame->IsString()) && !(stack_frame->IsArray()) && !(stack_frame == &root_))
        {
            throw std::logic_error(err_msg);
        }
        return;
    }

    bool Builder::IsReadyCheck()
    {
        if (nodes_stack_.back() == &root_ && !root_.IsNull())
        {
            return true;
        }
        return false;
    }
    /*
    void Builder::AddValueToParentNode(const Node* node_to_add, Node* parent_node)
    {
        if (parent_node == &root_)
        {
            root_ = Node(node_to_add->GetValue());
        } else if (parent_node->IsArray())
        {

        } else if (parent_node->IsString())
        {
            Dict& dict = std::get<Dict>(parent_node->GetValue());
            dict
        }
    }*/
}