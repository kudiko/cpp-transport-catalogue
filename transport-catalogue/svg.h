#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg
{

    struct Rgb
    {
        Rgb() = default;
        Rgb(uint8_t r, uint8_t g, uint8_t b);
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba
    {
        Rgba() = default;
        Rgba(uint8_t r, uint8_t g, uint8_t b, double a);
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    struct ColorToOstream
    {
        std::ostream& out;
        void operator()(std::monostate) const;
        void operator()(const std::string color_string) const;
        void operator()(const Rgb &rgb_struct) const;
        void operator()(const Rgba &rgba_struct) const;
    };

    // std::ostream& operator<<(std::ostream& out, const Rgb& rgb_struct);
    // std::ostream& operator<<(std::ostream& out, const Rgba& rgba_struct);

    
    using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
    inline Color NoneColor;

    std::ostream &operator<<(std::ostream &out, const Color &color);

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */

    enum class StrokeLineCap
    {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin
    {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream &operator<<(std::ostream &out, StrokeLineCap stroke);
    std::ostream &operator<<(std::ostream &out, StrokeLineJoin stroke);

    struct RenderContext;

    class Object
    {
    public:
        void Render(const RenderContext &context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext &context) const = 0;
    };

    class ObjectContainer
    {
    public:
        template <typename Obj>
        void Add(Obj obj);

        virtual void AddPtr(std::unique_ptr<Object> &&obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Drawable
    {
    public:
        virtual void Draw(ObjectContainer &objcont) const = 0;

        virtual ~Drawable() = default;
    };

    template <typename Owner>
    class PathProps
    {
    public:
        Owner &SetFillColor(Color color);
        Owner &SetStrokeColor(Color color);
        Owner &SetStrokeWidth(double width);
        Owner &SetStrokeLineCap(StrokeLineCap line_cap);
        Owner &SetStrokeLineJoin(StrokeLineJoin line_join);

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream &out) const
        {
            using namespace std::literals;

            if (fill_color_)
            {
                out << " fill=\""sv << fill_color_.value() << "\""sv;
            }
            if (stroke_color_)
            {
                out << " stroke=\""sv << stroke_color_.value() << "\""sv;
            }
            if (stroke_width_)
            {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (line_cap_)
            {
                out << " stroke-linecap=\""sv;
                out << line_cap_.value();
                out << "\""sv;
            }
            if (line_join_)
            {
                out << " stroke-linejoin=\""sv;
                out << line_join_.value();
                out << "\""sv;
            }
        }

    private:
        Owner &AsOwner();

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> line_cap_;
        std::optional<StrokeLineJoin> line_join_;
    };

    struct Point
    {
        Point() = default;
        Point(double x, double y)
            : x(x), y(y)
        {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext
    {
        RenderContext(std::ostream &out)
            : out(out)
        {
        }

        RenderContext(std::ostream &out, int indent_step, int indent = 0)
            : out(out), indent_step(indent_step), indent(indent)
        {
        }

        RenderContext Indented() const
        {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const
        {
            for (int i = 0; i < indent; ++i)
            {
                out.put(' ');
            }
        }

        std::ostream &out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle>
    {
    public:
        Circle &SetCenter(Point center);
        Circle &SetRadius(double radius);

    private:
        void RenderObject(const RenderContext &context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline>
    {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline &AddPoint(Point point);

    private:
        void RenderObject(const RenderContext &context) const override;
        std::vector<Point> polyline_points_;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text>
    {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text &SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text &SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text &SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text &SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text &SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text &SetData(std::string data);

    private:
        void RenderObject(const RenderContext &context) const override;

        std::string PrepateTextForSVG() const;

        Point starting_point_;
        Point offset_;
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class Document : public ObjectContainer
    {
    public:
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */

        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object> &&obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream &out) const;

    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };

    template <typename Obj>
    void ObjectContainer::Add(Obj obj)
    {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

    template <typename Owner>
    Owner &PathProps<Owner>::SetFillColor(Color color)
    {
        fill_color_ = color;
        return static_cast<Owner &>(*this);
    }

    template <typename Owner>
    Owner &PathProps<Owner>::SetStrokeColor(Color color)
    {
        stroke_color_ = color;
        return static_cast<Owner &>(*this);
    }

    template <typename Owner>
    Owner &PathProps<Owner>::SetStrokeWidth(double width)
    {
        stroke_width_ = width;
        return static_cast<Owner &>(*this);
    }

    template <typename Owner>
    Owner &PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap)
    {
        line_cap_ = line_cap;
        return static_cast<Owner &>(*this);
    }

    template <typename Owner>
    Owner &PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join)
    {
        line_join_ = line_join;
        return static_cast<Owner &>(*this);
    }

} // namespace svg