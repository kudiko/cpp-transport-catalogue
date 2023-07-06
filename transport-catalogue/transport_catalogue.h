#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>
#include <set>

#include "geo.h"
#include "domain.h"

namespace TransportInformator
{

namespace Core
{

class TransportCatalogue
{
    public:
    TransportCatalogue() = default;

    void AddStop(std::string_view name, detail::Coordinates coords);

    Stop* FindStop(std::string_view name) const;

    void AddBus(std::string_view name, const std::vector<std::string>& stop_names, bool is_roundtrip);

    Bus* FindBus(std::string_view name) const;

    std::optional<BusInfo> GetBusInfo(std::string_view name) const;
    std::optional<StopInfo> GetStopInfo(std::string_view name) const;

    void SetDistanceBetweenStops(const Stop* from, const Stop* to, double distance);

    double GetDistanceBetweenStops(const Stop* from, const Stop* to) const;

    std::set<std::string_view> GetAllNonEmptyBuses() const;
    std::set<std::string_view> GetAllNonEmptyStops() const;

    std::vector<detail::Coordinates> GetAllNonEmptyStopsCoords() const;

    
    std::set<std::string_view> GetBusesForStop(std::string_view stop_name) const;
    std::vector<detail::Coordinates> GetBusStopCoordsForBus(std::string_view bus_name) const;
    

    private:
    
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> stops_index_;

    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> buses_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;

    struct PairStopsHasher
    {
        size_t operator()(const std::pair<const Stop*, const Stop*>& pair) const
        {
            return std::hash<const Stop*>{}(pair.first) + 41 * std::hash<const Stop*>{}(pair.second);
        }
    };

    std::unordered_map<std::pair<const Stop*, const Stop*>, double, PairStopsHasher> distances_;
    
};

} // namespace TransportInformator::Core

} // namespace TransportInformator