#include "json.h"


#include <sstream>
#include <cassert> 
using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);
std::string LoadString(std::istream& input);

Array LoadArray(istream& input) {
    Array result;

    char c;
    while (input >> c && c != ']') {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']')
    {
        throw ParsingError("No closing brace for array");
    }
    return result;
}

Dict LoadDict(istream& input) {
    Dict result;

    char c;
    while (input >> c && c != '}') {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input);
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (c != '}')
    {
        throw ParsingError("No closing brace for dictionary");
    }

    return result;
}

bool LoadBool(std::istream& input)
{
    std::string s;
    
    while (isalpha(input.peek()))
    {
        s += input.get();
    }
    
    if (s == "true")
    {
        return true;
    } else if (s == "false")
    {
        return false;
    } else {
        throw ParsingError("Invalid bool value"s);
    }
}

std::nullptr_t LoadNull(std::istream& input)
{
    std::string s;
    while (isalpha(input.peek()))
    {
        s += input.get();
    }

    if (s == "null")
    {
        return {};
    } else {
        throw ParsingError("Invalid null value"s);
    }
}

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }

}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadNode(istream& input) {
    char c;
    input >> c;
    while (c == '\t' || c == '\r' || c == '\n' || c == ' ')
    {
        input >> c;
    }

    if (c == '[') {
        return {LoadArray(input)};
    } else if (c == '{') {
        return {LoadDict(input)};
    } else if (c == '\"') {
        return {LoadString(input)};
    } else if (c == 't' || c == 'f'){
        input.putback(c);
        return {LoadBool(input)};
    } else if (c == 'n'){
        input.putback(c);
        return {LoadNull(input)};
    } else {
        input.putback(c);
        Number number = LoadNumber(input);
        if (std::holds_alternative<int>(number))
        {
            return {std::get<int>(number)};
        } else {
            return {std::get<double>(number)};
        }
        
    }
}

}  // namespace

Node::Node(std::nullptr_t){}

Node::Node(int value)
{
    value_ = value;
}
    
Node::Node(double value)
{
    value_ = value;
}

Node::Node(bool value)
{
    value_ = value;
}

Node::Node(Array value)
{
    value_ = value;
}

Node::Node(Dict value)
{
    value_ = value;

}

Node::Node(std::string value)
{
    value_ = value;
}

bool Node::IsInt() const
{
    return std::holds_alternative<int>(value_);
}

bool Node::IsDouble() const
{
    return (std::holds_alternative<int>(value_) || 
     std::holds_alternative<double>(value_));
}

bool Node::IsPureDouble() const
{
    return std::holds_alternative<double>(value_);
}

bool Node::IsBool() const
{
    return std::holds_alternative<bool>(value_);
}

bool Node::IsString() const
{
    return std::holds_alternative<std::string>(value_);
}

bool Node::IsNull() const
{
    return std::holds_alternative<std::nullptr_t>(value_);
}

bool Node::IsArray() const
{
    return std::holds_alternative<Array>(value_);
}
    
bool Node::IsMap() const
{
    return std::holds_alternative<Dict>(value_);
}

int Node::AsInt() const
{
    if (!IsInt())
    {
        throw std::logic_error("Stored value isn't int");
    }
    return std::get<int>(value_);    
}
    
bool Node::AsBool() const
{
    if (!IsBool())
    {
        throw std::logic_error("Stored value isn't bool");
    }
    return std::get<bool>(value_);    
}

double Node::AsDouble() const
{
    if (!IsDouble())
    {
        throw std::logic_error("Stored value isn't double");
    }
    if (IsInt())
    {
        int result = std::get<int>(value_); 
        return result;
    } else {
        return std::get<double>(value_);    
    }
}

const std::string& Node::AsString() const
{
    if (!IsString())
    {
        throw std::logic_error("Stored value isn't string");
    }
    return std::get<std::string>(value_);  
}

const Array& Node::AsArray() const
{
    if (!IsArray())
    {
        throw std::logic_error("Stored value isn't array");
    }
    return std::get<Array>(value_);  
}

const Dict& Node::AsMap() const
{
    if (!IsMap())
    {
        throw std::logic_error("Stored value isn't map");
    }
    return std::get<Dict>(value_);  
}

const Node::Value& Node::GetValue() const
{
    return value_;
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    json::PrintNode(doc.GetRoot(), output);
}

bool operator==(const Node& lhs, const Node& rhs)
{
    return lhs.GetValue() == rhs.GetValue();
}

bool operator!=(const Node& lhs, const Node& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const Document& lhs, const Document& rhs)
{
    return lhs.GetRoot() == rhs.GetRoot();
}

bool operator!=(const Document& lhs, const Document& rhs)
{
    return lhs.GetRoot() != rhs.GetRoot();
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, std::ostream& out) 
{
    out << "null"sv;
}

void PrintValue(const std::string& value, std::ostream& out) {
    out << "\"";
    for (char c: value)
    {
        switch (c)
        {
        case '\\':
            out << '\\' << '\\';
            break;
        case '\"':
            out << '\\' << '\"';
            break;
        case '\n':
            out << '\\' << 'n';
            break;
        case '\r':
            out << '\\' << 'r';
            break;
        default:
            out << c;
            break;
        }
    }
    out << "\"";
}

void PrintValue(bool value, std::ostream& out) {
    if (value)
    {
        out << "true";
    } else {
        out << "false";
    }
}

void PrintValue(Array arr, std::ostream& out)
{
    out << '[';
    for (auto it = arr.begin(); it != arr.end(); ++it)
    {
        if (it != arr.begin())
        {
            out << ", ";
        }
        PrintNode(*it, out);
    }
    out << ']';
}

void PrintValue(Dict dict, std::ostream& out)
{
    out << '{';
    for (auto it = dict.begin(); it != dict.end(); ++it)
    {
        if (it != dict.begin())
        {
            out << ", ";
        }
        out << '\"' << it->first << '\"' << " : "; 
        PrintNode(it->second, out);
    }
    out << '}';
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit(
        [&out](const auto& value){ PrintValue(value, out); },
        node.GetValue());
} 

void TestTest()
{
    Node str_node{"Hello, \"everybody\""s};
    assert(str_node.IsString());
    assert(str_node.AsString() == "Hello, \"everybody\""s);
    std::stringstream strm;
    strm << "Hello, \"everybody\""s;
    assert(str_node.AsString() == Load(strm).GetRoot().AsString());
}


}  // namespace json