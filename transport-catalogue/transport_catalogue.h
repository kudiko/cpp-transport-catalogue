#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>
#include <set>

#include "geo.h"

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
    std::vector<Stop*> stops;
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

class TransportCatalogue
{
    public:
    TransportCatalogue() = default;

    void AddStop(std::string_view name, detail::Coordinates coords);

    Stop* FindStop(std::string_view name) const;

    void AddBus(std::string_view name, const std::vector<std::string>& stop_names);

    Bus* FindBus(std::string_view name) const;

    std::optional<BusInfo> GetBusInfo(std::string_view name) const;
    std::optional<StopInfo> GetStopInfo(std::string_view name) const;

    void AddDistanceBetweenStops( Stop* from, Stop* to, double distance);

    double GetDistanceBetweenStops (Stop* from, Stop* to) const;

    private:
    
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> stops_index_;

    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> buses_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;

    struct PairStopsHasher
    {
        size_t operator()(const std::pair<Stop*, Stop*>& pair) const
        {
            return std::hash<Stop*>{}(pair.first) + 41 * std::hash<Stop*>{}(pair.second);
        }
    };

    std::unordered_map<std::pair<Stop*, Stop*>, double, PairStopsHasher> distances_;
    
};

} // namespace TransportInformator::Core

} // namespace TransportInformator