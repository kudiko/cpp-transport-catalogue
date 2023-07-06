#include "map_renderer.h"

#include <algorithm>
/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace TransportInformator
{

    namespace Render
    {

        bool IsZero(double value)
        {
            return std::abs(value) < EPSILON;
        }

        // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
        template <typename PointInputIt>
        SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                                         double max_width, double max_height, double padding)
            : padding_(padding) //
        {
            // Если точки поверхности сферы не заданы, вычислять нечего
            if (points_begin == points_end)
            {
                return;
            }

            // Находим точки с минимальной и максимальной долготой
            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs)
                { return lhs.lng < rhs.lng; });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            // Находим точки с минимальной и максимальной широтой
            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs)
                { return lhs.lat < rhs.lat; });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            // Вычисляем коэффициент масштабирования вдоль координаты x
            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_))
            {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            // Вычисляем коэффициент масштабирования вдоль координаты y
            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat))
            {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom)
            {
                // Коэффициенты масштабирования по ширине и высоте ненулевые,
                // берём минимальный из них
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom)
            {
                // Коэффициент масштабирования по ширине ненулевой, используем его
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom)
            {
                // Коэффициент масштабирования по высоте ненулевой, используем его
                zoom_coeff_ = *height_zoom;
            }
        }

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point SphereProjector::operator()(TransportInformator::detail::Coordinates coords) const
        {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
        }

        MapRenderer::MapRenderer(const RenderSettings& settings, const std::vector<detail::Coordinates>& all_coords) 
        : settings_{settings}, bus_color_it{settings_.color_palette.begin()}, 
        projector_{all_coords.begin(), all_coords.end(), settings_.width, settings_.height, settings_.padding}
        {}

        void MapRenderer::DrawBus(const BusDrawingInfo& bus_info)
        {
            svg::Polyline new_bus;

            if (bus_info.is_roundtrip)
            {
                for (const auto& stop_coords : bus_info.stops_coords)
                {
                    new_bus.AddPoint(projector_(stop_coords));
                }
            } else {
                for (const auto& stop_coords : bus_info.stops_coords)
                {
                    new_bus.AddPoint(projector_(stop_coords));
                }
                
                for (auto it = std::next(bus_info.stops_coords.end(), -2); it != std::next(bus_info.stops_coords.begin(), -1); --it)
                {
                    new_bus.AddPoint(projector_(*it));
                }
            }
            
            
            new_bus.SetStrokeColor(*bus_color_it).SetFillColor(svg::NoneColor).SetStrokeWidth(settings_.line_width);
            new_bus.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);


            doc_.Add(new_bus);

            bus_to_color_[bus_info.name] = *bus_color_it;
            MoveCurrentBusColor();
        }

        void MapRenderer::DrawBusLabel(const BusLabelInfo& bus_info)
        {
            svg::Text start_label;

            start_label.SetPosition(projector_(bus_info.start_coords)).SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
            start_label.SetFontSize(settings_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold");
            start_label.SetData(static_cast<std::string>(bus_info.bus_name));

            svg::Text start_label_underlayer = start_label;

            start_label_underlayer.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color);
            start_label_underlayer.SetStrokeWidth(settings_.underlayer_width);
            start_label_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            start_label.SetFillColor(bus_to_color_.at(bus_info.bus_name));

            // подложку рисуем сначала
            doc_.Add(start_label_underlayer);
            doc_.Add(start_label);

            if (bus_info.start_coords != bus_info.finish_coords)
            {
                svg::Text finish_label;

                finish_label.SetPosition(projector_(bus_info.finish_coords)).SetOffset({settings_.bus_label_offset[0], settings_.bus_label_offset[1]});
                finish_label.SetFontSize(settings_.bus_label_font_size).SetFontFamily("Verdana").SetFontWeight("bold");
                finish_label.SetData(static_cast<std::string>(bus_info.bus_name));

                svg::Text finish_label_underlayer = finish_label;

                finish_label_underlayer.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color);
                finish_label_underlayer.SetStrokeWidth(settings_.underlayer_width);
                finish_label_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

                finish_label.SetFillColor(bus_to_color_.at(bus_info.bus_name));

                doc_.Add(finish_label_underlayer);
                doc_.Add(finish_label);
                
            }

        }

        void MapRenderer::DrawStopSymbol(const Core::Stop* stop)
        {
            svg::Circle stop_symbol;
            stop_symbol.SetCenter(projector_(stop->coords)).SetRadius(settings_.stop_radius);
            stop_symbol.SetFillColor("white");

            doc_.Add(stop_symbol);
        }

        void MapRenderer::DrawStopLabel(const Core::Stop* stop)
        {
            svg::Text stop_label;

            stop_label.SetPosition(projector_(stop->coords)).SetOffset({settings_.stop_label_offset[0], settings_.stop_label_offset[1]});
            stop_label.SetFontSize(settings_.stop_label_font_size).SetFontFamily("Verdana");
            stop_label.SetData(stop->name);

            svg::Text stop_label_underlayer = stop_label;
            stop_label_underlayer.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color);
            stop_label_underlayer.SetStrokeWidth(settings_.underlayer_width);
            stop_label_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            stop_label.SetFillColor("black");

            doc_.Add(stop_label_underlayer);
            doc_.Add(stop_label);

        }

        void MapRenderer::MoveCurrentBusColor()
        {
            if (++bus_color_it == settings_.color_palette.end())
            {
                bus_color_it = settings_.color_palette.begin();
            }
        }
        
        const svg::Document& MapRenderer::GetDocument() const
        {
            return doc_;
        }

    } // namespace TransportInformator::Render

} // namespace TransportInformator