#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"
#include <variant>
#include <unordered_map>

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace TransportInformator
{

namespace Render
{



inline const double EPSILON = 1e-6;

bool IsZero(double value);

struct RenderSettings
{
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    size_t bus_label_font_size;
    std::array<double, 2> bus_label_offset;
    size_t stop_label_font_size;
    std::array<double, 2> stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;
};

struct BusDrawingInfo
{
    std::string_view name;
    std::vector<detail::Coordinates> stops_coords;
    bool is_roundtrip;
};

struct BusLabelInfo
{
    std::string_view bus_name;
    detail::Coordinates start_coords;
    detail::Coordinates finish_coords;

};

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);
    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(TransportInformator::detail::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer
{
    public:
    MapRenderer(const RenderSettings& settings, const std::vector<detail::Coordinates>& all_coords);
    void DrawBus(const BusDrawingInfo& bus_info);
    void DrawBusLabel(const BusLabelInfo& bus_info);
    void DrawStopSymbol(const Core::Stop* stop);
    void DrawStopLabel(const Core::Stop* stop);

    const svg::Document& GetDocument() const;

    private:
    void MoveCurrentBusColor();

    std::unordered_map<std::string_view, svg::Color> bus_to_color_;

    RenderSettings settings_;
    std::vector<svg::Color>::const_iterator bus_color_it;
    SphereProjector projector_;

    svg::Document doc_;
    
};

} // namespace TransportInformator::Render

} //namespace TransportInformator