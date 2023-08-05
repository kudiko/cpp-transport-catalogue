#include "request_handler.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include <sstream>
#include <algorithm>
#include <iomanip>




namespace TransportInformator
{

    namespace ReqHandler
    {

        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler::RequestHandler(const Core::TransportCatalogue& db, Render::MapRenderer& renderer, Router::TransportRouter& router) 
        : db_{db}, renderer_{renderer}, router_{router}{}

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<Core::BusInfo> RequestHandler::GetBusStat(const std::string_view &bus_name) const
        {
            return db_.GetBusInfo(bus_name);
        }

        // Возвращает маршруты, проходящие через остановку
        const std::set<std::string_view> RequestHandler::GetBusesByStop(const std::string_view &stop_name) const
        {
            return db_.GetBusesForStop(stop_name);
        }

        // Этот метод будет нужен в следующей части итогового проекта
        const svg::Document& RequestHandler::RenderMap()
        {
            
            std::vector<Render::BusDrawingInfo> bus_draw_info;

            const auto buses_to_draw = db_.GetAllNonEmptyBuses();
            for (const auto& bus_name : buses_to_draw)
            {
                const Core::Bus* bus_ptr = db_.FindBus(bus_name);
                const auto start = bus_ptr->stops.front();
                const auto finish = bus_ptr->stops.back();
                bus_draw_info.push_back({bus_ptr->name, db_.GetBusStopCoordsForBus(bus_name), bus_ptr->is_roundtrip, start->coords, finish->coords});
            }

            std::vector<const Core::Stop*> stop_draw_info;

            const auto stops_to_draw = db_.GetAllNonEmptyStops();

            for (const auto& stop_name : stops_to_draw)
            {
                stop_draw_info.push_back({db_.FindStop(stop_name)});
            }

            return renderer_.RenderMap(bus_draw_info, stop_draw_info);   
        }

        Router::Route RequestHandler::BuildRoute(std::string_view from, std::string_view to) const
        {
            if (!router_.GraphHasValue())
            {
                router_.BuildGraph();
            }
            if (!router_.RouterHasValue())
            {
                router_.InitRouter();
            }
            return router_.BuildRoute(from, to);
        }

    }





} // namespace TransportInformator