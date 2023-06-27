#include "transport_catalogue.h"
#include "geo.h"

#include <iostream>
#include <set>
#include <numeric>
#include <cassert>

namespace TransportInformator
{

namespace Core
{

void TransportCatalogue::AddStop(std::string_view name, detail::Coordinates coords)
{
    if (stops_index_.count(name))
    {
        Stop *stop_to_edit = FindStop(name);
        stop_to_edit->coords = coords;
        return;
    }
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

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string> &stop_names)
{
    std::vector<Stop *> stops_pointers;
    stops_pointers.reserve(stop_names.size());
    for (const std::string &name : stop_names)
    {
        Stop *stop_ptr = FindStop(name);
        stops_pointers.push_back(stop_ptr);
    }

    buses_.push_back({std::string(name), stops_pointers});
    std::string_view new_bus_name(buses_.back().name);
    buses_index_[new_bus_name] = &buses_.back();

    for (const Stop *stop : buses_.back().stops)
    {
        stops_to_buses_[stop->name].insert(new_bus_name);
    }
}

Bus *TransportCatalogue::FindBus(std::string_view name) const
{
    assert(buses_index_.count(name));
    return buses_index_.at(name);
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const
{
    if (!buses_index_.count(name))
    {
        return {};
    }

    Bus &bus_ref = *buses_index_.at(name);

    std::set<Stop *> unique_stops{bus_ref.stops.begin(), bus_ref.stops.end()};

    double geographic_distance = 0.0;
    double real_distance = 0.0;

    for (auto it = std::next(bus_ref.stops.begin(), 1); it != bus_ref.stops.end(); ++it)
    {
        geographic_distance += ComputeDistance((*it)->coords, (*(std::next(it, -1)))->coords);
        real_distance += GetDistanceBetweenStops(*std::next(it, -1), *it);
    }

    double curvature = real_distance / geographic_distance;

    BusInfo result{bus_ref.name, bus_ref.stops.size(), unique_stops.size(), real_distance, curvature};
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

void TransportCatalogue::AddDistanceBetweenStops(Stop *from, Stop *to, double distance)
{
    distances_[{from, to}] = distance;

    if (!distances_.count({to, from}))
    {
        distances_[{to, from}] = distance;
    }
}

double TransportCatalogue::GetDistanceBetweenStops(Stop *from, Stop *to) const
{
    assert(distances_.count({from, to}));
    return distances_.at({from, to});
}

} // namespace TransportInformator::Core

} // namespace TransportInformator