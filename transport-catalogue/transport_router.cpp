#include "transport_router.h"

#include <iostream>
namespace TransportInformator
{

namespace Router
{

TransportRouter::TransportRouter(const Core::TransportCatalogue& tc, TransportRouterParameters pars) : 
bus_wait_time_{pars.bus_wait_time}, bus_velocity_{pars.bus_velocity}, tc_{tc}, graph_{std::nullopt}, router_{std::nullopt}
{
    const std::set<std::string_view> all_stops = tc_.GetAllStops();
    size_t vertice_id = 0;
    for (const auto stop_name : all_stops)
    {
        vertice_id_to_stop_name_[vertice_id] = stop_name;
        vertice_id_to_stop_name_[vertice_id + 1] = stop_name;

        stop_name_to_vertice_ids_[stop_name] = {vertice_id, vertice_id + 1};

        vertice_id += 2;
    }
}

void TransportRouter::BuildGraph()
{
    
    graph::DirectedWeightedGraph<double> new_graph(vertice_id_to_stop_name_.size());
    graph_ = new_graph;

    for (size_t i = 0; i < vertice_id_to_stop_name_.size(); i += 2)
    {
        graph_.value().AddEdge({i, i + 1, static_cast<double>(bus_wait_time_)}); // wait edge
    }

    const std::set<std::string_view> all_buses = tc_.GetAllNonEmptyBuses();
    for (const auto& bus_name : all_buses)
    {
        auto bus_ptr = tc_.FindBus(bus_name);
        assert(bus_ptr->stops.size() > 1);

        if (bus_ptr->is_roundtrip)
        {
            MakeEdgesForBus(bus_ptr->stops.begin(), bus_ptr->stops.end(), bus_name);
        }

        if (!(bus_ptr->is_roundtrip))
        {
            MakeEdgesForBus(bus_ptr->stops.begin(), bus_ptr->stops.end(), bus_name);
            MakeEdgesForBus(bus_ptr->stops.rbegin(), bus_ptr->stops.rend(), bus_name);
        }
    }
}

bool TransportRouter::GraphHasValue() const
{
    return graph_.has_value();
}

bool TransportRouter::RouterHasValue() const
{
    return router_.has_value();
}

void TransportRouter::InitRouter()
{
    router_.emplace(graph_.value());
}
Route TransportRouter::BuildRoute(std::string_view from, std::string_view to) const
{
    const size_t from_id = stop_name_to_vertice_ids_.at(from).enter_bus_vertex;
    const size_t to_id = stop_name_to_vertice_ids_.at(to).enter_bus_vertex;

    const std::optional<graph::Router<double>::RouteInfo> BuildRouteResult = router_.value().BuildRoute(from_id, to_id);

    if (!BuildRouteResult.has_value())
    {
        return {};
    }

    return ProcessRouteInfo(BuildRouteResult.value());
}

Route TransportRouter::ProcessRouteInfo(const graph::Router<double>::RouteInfo& route_info) const
{
    double total_time = route_info.weight;
    std::vector<std::variant<Route::RouteElementWait, Route::RouteElementBus>> result;

    assert(graph_.has_value());

    for (auto it = route_info.edges.begin(); it != route_info.edges.end(); ++it)
    {
        const graph::Edge<double>& current_edge = graph_->GetEdge(*it);
        if (!edge_id_to_bus_name_.count(*it))
        {
            result.emplace_back(Route::RouteElementWait{static_cast<std::string>(vertice_id_to_stop_name_.at(current_edge.from)), current_edge.weight});
            continue;
        }


        std::string_view bus_name = edge_id_to_bus_name_.at(*it);
        double time = current_edge.weight;
         /*
        std::string_view from = vertice_id_to_stop_name_.at(current_edge.from);
        std::string_view to = vertice_id_to_stop_name_.at(current_edge.to);


        const Core::Stop* from_ptr = tc_.FindStop(from);
        const Core::Stop* to_ptr = tc_.FindStop(to);

        const Core::Bus* bus_ptr = tc_.FindBus(bus_name);

        auto from_it = std::find(bus_ptr->stops.begin(), bus_ptr->stops.end(), from_ptr);
        auto to_it = std::find(from_it, bus_ptr->stops.end(), to_ptr);
        */

        int span_count = edge_td_to_span_count_.at(*it);
            
        result.emplace_back(Route::RouteElementBus{static_cast<std::string>(bus_name), span_count, time});
    }
    return {total_time, std::move(result)};
}



template <class InputIt>
void TransportRouter::MakeEdgesForBus(InputIt stops_begin, InputIt stops_end, std::string_view bus_name)
{
    for (auto outer_it = stops_begin; outer_it != stops_end; ++outer_it)
    {
        double time_from_start = 0.;
        int span_count = 0;
        for (auto it = std::next(outer_it, 1); it != stops_end; ++it)
        {
            std::string_view stop_name_from((*outer_it)->name);
            std::string_view stop_name_to((*it)->name);

            size_t leave_vertex_id = stop_name_to_vertice_ids_.at(stop_name_from).leave_bus_vertex;
            size_t enter_vertex_id = stop_name_to_vertice_ids_.at(stop_name_to).enter_bus_vertex;

            double time_of_ride_between_stops = tc_.GetDistanceBetweenStops(*std::next(it, -1), *it) / (bus_velocity_ / 3600 * 1000) / 60; // last division is conversion to minutes
            time_from_start += time_of_ride_between_stops;

            size_t new_bus_edge = graph_.value().AddEdge({leave_vertex_id, enter_vertex_id, time_from_start});
            edge_td_to_span_count_[new_bus_edge] = ++span_count;
            edge_id_to_bus_name_[new_bus_edge] = bus_name;
        }
    }

}

} // namespace Router



}// namespace TransportInformator