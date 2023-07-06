#include "svg.h"

#include <sstream>

namespace svg
{

    using namespace std::literals;

    Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}

    Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double a) : red(r), green(g), blue(b), opacity(a) {}

    void ColorToOstream::operator()(std::monostate) const { out << "none"; }
    void ColorToOstream::operator()(const std::string color_string) const { out << color_string; }
    void ColorToOstream::operator()(const Rgb &rgb_struct) const
    {
        out << "rgb(" << std::to_string(rgb_struct.red) << "," << std::to_string(rgb_struct.green) << ","
           << std::to_string(rgb_struct.blue) << ")";
    }

    void ColorToOstream::operator()(const Rgba &rgba_struct) const
    {
        out << "rgba(" << std::to_string(rgba_struct.red) << ',' << std::to_string(rgba_struct.green)
           << ',' << std::to_string(rgba_struct.blue) << ',' << rgba_struct.opacity << ')';
    }

    void Object::Render(const RenderContext &context) const
    {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle &Circle::SetCenter(Point center)
    {
        center_ = center;
        return *this;
    }

    Circle &Circle::SetRadius(double radius)
    {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext &context) const
    {
        auto &out = context.out;
        out << "  <circle";
        
        out << " cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        
        out << "/>"sv;
    }

    // ----------- Polyline ---------------

    Polyline &Polyline::AddPoint(Point point)
    {
        polyline_points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext &context) const
    {
        auto &out = context.out;
        out << "  <polyline";

        
        out << " points=\"";

        for (auto it = polyline_points_.begin(); it != polyline_points_.end(); ++it)
        {
            out << it->x << ',' << it->y;
            if (it != std::prev(polyline_points_.end()))
            {
                out << ' ';
            }
        }

        out << "\"";

        RenderAttrs(out);
        
        out << "/>";
    }

    // ----------------- Text -----------------------

    // Задаёт координаты опорной точки (атрибуты x и y)
    Text &Text::SetPosition(Point pos)
    {
        starting_point_ = pos;
        return *this;
    }

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text &Text::SetOffset(Point offset)
    {
        offset_ = offset;
        return *this;
    }

    // Задаёт размеры шрифта (атрибут font-size)
    Text &Text::SetFontSize(uint32_t size)
    {
        font_size_ = size;
        return *this;
    }

    // Задаёт название шрифта (атрибут font-family)
    Text &Text::SetFontFamily(std::string font_family)
    {
        font_family_ = font_family;
        return *this;
    }

    // Задаёт толщину шрифта (атрибут font-weight)
    Text &Text::SetFontWeight(std::string font_weight)
    {
        font_weight_ = font_weight;
        return *this;
    }

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text &Text::SetData(std::string data)
    {
        data_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext &context) const
    {
        auto &out = context.out;
        
        out << "  <text";
        RenderAttrs(out);
        out << " x=\"" << starting_point_.x << "\" y=\"" << starting_point_.y << "\" ";
        out << "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" ";
        out << "font-size=\"" << font_size_ << "\"";
        if (!font_family_.empty())
        {
            out << " font-family=\"" << font_family_ << "\"";
        }
        

        if (!font_weight_.empty())
        {
            out << " font-weight=\"" << font_weight_ << "\"";
        }
        
        out << ">";
        out << PrepateTextForSVG();

        out << "</text>";
    }

    std::string Text::PrepateTextForSVG() const
    {
        std::stringstream ss;

        for (const char &c : data_)
        {
            switch (c)
            {
            case '\"':
                ss << "&quot;";
                break;
            case '\'':
                ss << "&apos;";
                break;
            case '<':
                ss << "&lt;";
                break;
            case '>':
                ss << "&gt;";
                break;
            case '&':
                ss << "&amp;";
                break;
            default:
                ss << c;
                break;
            }
        }
        return ss.str();
    }
    // -----------------Document--------------------
    /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */

    // Добавляет в svg-документ объект-наследник svg::Object
    void Document::AddPtr(std::unique_ptr<Object> &&obj)
    {
        objects_.push_back(std::move(obj));
    }

    // Выводит в ostream svg-представление документа
    void Document::Render(std::ostream &out) const
    {
        RenderContext context{out};

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;

        for (const auto &obj : objects_)
        {
            obj->Render(context);
        }

        out << "</svg>";
    }

    std::ostream &operator<<(std::ostream &out, StrokeLineCap stroke)
    {
        switch (stroke)
        {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        default:
            break;
        }
        return out;
    }
    std::ostream &operator<<(std::ostream &out, StrokeLineJoin stroke)
    {
        switch (stroke)
        {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        default:
            break;
        }
        return out;
    }
    /*
    std::ostream& operator<<(std::ostream& out, const Rgb& s)
    {
        out << "rgb(" << s.red << ',' << s.green << ',' << s.blue << ')';
        return out;
    }
    std::ostream& operator<<(std::ostream& out, const Rgba& s)
    {
        out << "rgba(" << s.red << ',' << s.green << ',' << s.blue << ',' << s.opacity << ')';
        return out;
    }*/

    std::ostream &operator<<(std::ostream &out, const Color &color)
    {
        std::visit(ColorToOstream{out}, color);
        return out;
    }

} // namespace svg