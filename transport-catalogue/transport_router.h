#pragma once

#include <optional>
#include <variant>
#include <unordered_map>
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace TransportInformator
{

namespace Router
{

struct TransportRouterParameters
{
    TransportRouterParameters& SetBusWaitTime(int wait_time)
    {
        bus_wait_time = wait_time;
        return *this;
    }
    TransportRouterParameters& SetBusVelocity(double velocity)
    {
        bus_velocity = velocity;
        return *this;
    }

    int GetBusWaitTime() const
    {
        return bus_wait_time;
    }
    double GetBusVelocity() const
    {
        return bus_velocity;
    }

    private:
    friend class TransportRouter;
    int bus_wait_time = -1;
    double bus_velocity = -1;
};

struct Route
{
    double total_time = -1.;
    
    struct RouteElementWait
    {
        std::string stop_name;
        double time;
    };

    struct RouteElementBus
    {
        std::string bus;
        int span_count;
        double time;
    };

    std::vector<std::variant<RouteElementWait, RouteElementBus>> route_details;

};

class TransportRouter
{
    public:
    TransportRouter(const Core::TransportCatalogue& tc, TransportRouterParameters pars);
    TransportRouter(const Core::TransportCatalogue& tc, TransportRouterParameters pars,
                    const graph::DirectedWeightedGraph<double>& graph,
                    const graph::Router<double>& router,
                    std::unordered_map<size_t, std::string> edge_id_to_bus_name,
                    std::unordered_map<size_t, int> edge_id_to_span_count
                    );

    std::optional<Route> BuildRoute(std::string_view from, std::string_view to) const;

    graph::DirectedWeightedGraph<double> GetGraph() const
    {
        assert(graph_.has_value());
        return graph_.value();
    }
    graph::Router<double> GetRouter() const
    {
        assert(router_.has_value());
        return router_.value();
    }
    std::unordered_map<size_t, std::string> GetIdToBusName() const
    {
        return edge_id_to_bus_name_;
    }
    std::unordered_map<size_t, int> GetIdToSpanCount() const
    {
        return edge_id_to_span_count_;
    }


    private:

    void BuildGraph();
    void InitRouter();

    struct StopVertices
    {
        size_t enter_bus_vertex; // all buses go here
        size_t leave_bus_vertex; // all buses leave from here

    };

    int bus_wait_time_;
    double bus_velocity_;

    std::unordered_map<size_t, std::string_view> vertice_id_to_stop_name_;
    std::unordered_map<std::string_view, StopVertices> stop_name_to_vertice_ids_;

    const Core::TransportCatalogue& tc_;
    std::optional<graph::DirectedWeightedGraph<double>> graph_;
    std::optional<graph::Router<double>> router_;

    template <class InputIt>
    void MakeEdgesForBus(InputIt stops_begin, InputIt stops_end, std::string_view bus_name);

    Route ProcessRouteInfo(const graph::Router<double>::RouteInfo& route_info) const;

    std::unordered_map<size_t, std::string> edge_id_to_bus_name_;
    std::unordered_map<size_t, int> edge_id_to_span_count_;



};

} // namespace Router


} // namespace TransportInformator