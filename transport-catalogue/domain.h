#pragma once

#include <string>
#include <vector>
#include <set>
#include <string_view>

#include "geo.h"

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки. 
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

namespace TransportInformator
{

namespace Core
{

struct Stop
{
    std::string name;
    detail::Coordinates coords;
};

struct Bus
{
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

struct BusInfo
{
    std::string_view name;
    size_t stops;
    size_t unique_stops;
    double route_length;
    double curvature;
};

struct StopInfo
{
    std::string_view name;
    std::set<std::string_view> buses;
};

} //namespace Core

} //namespace TransportInformator