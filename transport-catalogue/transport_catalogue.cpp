#include "transport_catalogue.h"
#include "geo.h"

#include <iostream>
#include <set>
#include <numeric>

namespace TransportInformator
{

namespace Core
{

void TransportCatalogue::AddStop(std::string_view name, detail::Coordinates coords)
{
    stops_.push_back({std::string(name), coords});
    std::string_view new_stop_name(stops_.back().name);
    stops_index_[new_stop_name] = &stops_.back();
    stops_to_buses_[name];
}

Stop *TransportCatalogue::FindStop(std::string_view name) const
{
    if (!stops_index_.count(name))
    {
        return nullptr;
    }
    return stops_index_.at(name);
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string> &stop_names, bool is_roundtrip)
{
    std::vector<const Stop *> stops_pointers;
    stops_pointers.reserve(stop_names.size());
    for (const std::string &name : stop_names)
    {
        const Stop *stop_ptr = FindStop(name);
        stops_pointers.push_back(stop_ptr);
    }

    buses_.push_back({std::string(name), stops_pointers, is_roundtrip});
    std::string_view new_bus_name(buses_.back().name);
    buses_index_[new_bus_name] = &buses_.back();

    for (const Stop *stop : buses_.back().stops)
    {
        stops_to_buses_[stop->name].insert(new_bus_name);
    }
}

Bus* TransportCatalogue::FindBus(std::string_view name) const
{
    return buses_index_.at(name);
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const
{
    if (!buses_index_.count(name))
    {
        return {};
    }

    const Bus &bus_ref = *buses_index_.at(name);

    size_t number_of_stops = bus_ref.is_roundtrip ? bus_ref.stops.size() : 2 * bus_ref.stops.size() - 1;

    std::set<const Stop *> unique_stops{bus_ref.stops.begin(), bus_ref.stops.end()};

    double geographic_distance = 0.0;
    double real_distance = 0.0;

    if (bus_ref.is_roundtrip)
    {
        for (auto it = std::next(bus_ref.stops.begin(), 1); it != bus_ref.stops.end(); ++it)
        {
            geographic_distance += ComputeDistance((*it)->coords, (*(std::next(it, -1)))->coords);
            real_distance += GetDistanceBetweenStops(*std::next(it, -1), *it);
        }
    } else {
        for (auto it = std::next(bus_ref.stops.begin(), 1); it != bus_ref.stops.end(); ++it)
        {
            geographic_distance += ComputeDistance((*it)->coords, (*(std::next(it, -1)))->coords);
            real_distance += GetDistanceBetweenStops(*std::next(it, -1), *it);
        }

        for (auto it = std::next(bus_ref.stops.end(), -1); it != bus_ref.stops.begin(); --it)
        {
            geographic_distance += ComputeDistance((*it)->coords, (*(std::next(it, -1)))->coords);
            real_distance += GetDistanceBetweenStops(*it, *std::next(it, -1));
        }
    }

    

    double curvature = real_distance / geographic_distance;

    BusInfo result{bus_ref.name, number_of_stops, unique_stops.size(), real_distance, curvature};
    return result;
}

std::optional<StopInfo> TransportCatalogue::GetStopInfo(std::string_view name) const
{
    if (!stops_index_.count(name))
    {
        return {};
    }
    StopInfo result{name, stops_to_buses_.at(name)};
    return result;
}

void TransportCatalogue::SetDistanceBetweenStops(const Stop *from, const Stop *to, double distance)
{
    distances_[{from, to}] = distance;

    if (!distances_.count({to, from}))
    {
        distances_[{to, from}] = distance;
    }
}

double TransportCatalogue::GetDistanceBetweenStops(const Stop *from, const Stop *to) const
{
    return distances_.at({from, to});
}

std::set<std::string_view> TransportCatalogue::GetAllNonEmptyBuses() const
{
    std::set<std::string_view> all_buses;
    for (const auto& [bus_name, bus_ptr] : buses_index_)
    {
        if (!(bus_ptr->stops.empty()))
        {
            all_buses.insert(bus_name);
        }
    }
    return all_buses;
}

std::set<std::string_view> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const
{
    if (stops_to_buses_.count(stop_name))
    {
        return stops_to_buses_.at(stop_name);
    }
    return {};
}

std::vector<detail::Coordinates> TransportCatalogue::GetBusStopCoordsForBus(std::string_view bus_name) const
{
    std::vector<detail::Coordinates> result;
    if (buses_index_.count(bus_name))
    {
        Bus* bus_ref = buses_index_.at(bus_name);
        for (const auto& stop : bus_ref->stops)
        {
            result.push_back(stop->coords);
        }
        return result;
    }
    return {};
}

std::set<std::string_view> TransportCatalogue::GetAllNonEmptyStops() const
{
    std::set<std::string_view> result;

    for (const auto& [stop_name, stop_ptr] : stops_index_)
    {
        if (!stops_to_buses_.at(stop_name).empty())
        {
            result.insert(stop_name);
        }
    }
    return result;
}

std::vector<detail::Coordinates> TransportCatalogue::GetAllNonEmptyStopsCoords() const
{
    std::vector<detail::Coordinates> result;

    for (const auto& [stop_name, stop_ptr] : stops_index_)
    {
        if (!stops_to_buses_.at(stop_name).empty())
        {
            result.push_back(stop_ptr->coords);
        }
    }
    return result;
}

} // namespace TransportInformator::Core

} // namespace TransportInformator