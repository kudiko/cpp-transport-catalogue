#include "serialization.h"
#include <transport_catalogue.pb.h>

#include <fstream>
TransportInformator::Serialize::Serializator::Serializator(TransportInformator::Core::TransportCatalogue &tc,
                                                           TransportInformator::Serialize::SerializationParameters pars) :
                                                           tc_{tc}, pars_{std::move(pars)}{}

void TransportInformator::Serialize::Serializator::SerializeToFile
(const TransportInformator::Render::RenderSettings& render_settings,
 const TransportInformator::Router::TransportRouterParameters& router_parameters,
 const TransportInformator::Router::TransportRouter& transport_router)
{
    std::ofstream out(pars_.file, std::ios::binary);

    db_serialization::TCWithSettings result;

    db_serialization::TransportCatalogue* tc_ptr = result.mutable_tc();
    *tc_ptr = tc_.DumpDB();

    db_serialization::RenderSettings* render_settings_ptr = result.mutable_render_settings();
    *render_settings_ptr = SerializeRenderSettings(render_settings);

    db_serialization::TransportRouterParameters* router_settings_ptr = result.mutable_router_settings();
    *router_settings_ptr = SerializeRouterSettings(router_parameters);

    db_serialization::TransportRouter* tr_router_ptr = result.mutable_transport_router();

    db_serialization::Graph* graph_ptr = tr_router_ptr->mutable_graph();
    *graph_ptr = SerializeGraph(transport_router.GetGraph());

    db_serialization::RoutesInternalData* routes_internal_data_ptr = tr_router_ptr->mutable_internal_data();
    *routes_internal_data_ptr = SerializeRouter(transport_router.GetRouter());

    for (const auto& [id, bus_name] : transport_router.GetIdToBusName() )
    {
        db_serialization::EdgeIdToBusName* cur_id_to_bus_name = tr_router_ptr->add_id_to_bus_name();
        cur_id_to_bus_name->set_edge_id(id);
        cur_id_to_bus_name->set_bus_name(static_cast<std::string>(bus_name));
    }

    for (const auto& [id, span_count] : transport_router.GetIdToSpanCount())
    {
        db_serialization::EdgeIdToSpanCount* cur_id_to_span_count = tr_router_ptr->add_id_to_span_count();
        cur_id_to_span_count->set_edge_id(id);
        cur_id_to_span_count->set_span_count(span_count);
    }


    result.SerializeToOstream(&out);
}

TransportInformator::Router::TransportRouter TransportInformator::Serialize::Serializator::UnserializeFromFile()
{
    std::ifstream in(pars_.file, std::ios::binary);
    db_serialization::TCWithSettings read_db_and_settings;
    if (!read_db_and_settings.ParseFromIstream(&in))
    {
        assert(false);
    }

    auto read_db = read_db_and_settings.tc();

    int size_of_stops = read_db.stops().stops_size();

    std::unordered_map<int, std::string_view> id_to_stop_name;
    for (int i = 0; i < size_of_stops; ++i)
    {
        auto current_stop = read_db.stops().stops(i);
        tc_.AddStop(current_stop.name(), {current_stop.coords().lat(), current_stop.coords().long_()});
        id_to_stop_name[current_stop.id()] = tc_.FindStop(current_stop.name())->name;
    }

    int size_of_distances = read_db.distances().distances_size();
    for (int i = 0; i < size_of_distances; ++i)
    {
        auto current_distance = read_db.distances().distances(i);
        tc_.SetDistanceBetweenStops(tc_.FindStop(id_to_stop_name[current_distance.stop_from()]),
                                    tc_.FindStop(id_to_stop_name[current_distance.stop_to()]), current_distance.distance());
    }

    int size_of_buses = read_db.buses().buses_size();
    for (int i = 0; i < size_of_buses; ++i)
    {
        auto current_bus = read_db.buses().buses(i);

        std::vector<std::string> stop_names;
        int size_of_stops_in_this_bus = current_bus.stops_size();
        for (int j = 0; j < size_of_stops_in_this_bus; ++j)
        {
            std::string name(id_to_stop_name[current_bus.stops(j)]);
            stop_names.push_back(std::move(name));
        }

        tc_.AddBus(current_bus.name(), stop_names, current_bus.is_roundtrip());
    }

    render_setings_ = DeserializeRenderSettings(read_db_and_settings.render_settings());
    router_settings_ = DeserializeRouterSettings(read_db_and_settings.router_settings());

    graph_ = DeserializeGraph(read_db_and_settings.transport_router().graph());

    std::unordered_map<size_t, std::string> edge_id_to_bus_name;
    for (int i = 0; i < read_db_and_settings.transport_router().id_to_bus_name_size(); ++i)
    {
        auto cur_id_to_bus_pair = read_db_and_settings.transport_router().id_to_bus_name(i);
        edge_id_to_bus_name.insert({cur_id_to_bus_pair.edge_id(), cur_id_to_bus_pair.bus_name()});
    }



    std::unordered_map<size_t, int> edge_id_to_span_count;
    for (int i = 0; i < read_db_and_settings.transport_router().id_to_span_count_size(); ++i)
    {
        auto cur_id_to_span_count_pair = read_db_and_settings.transport_router().id_to_span_count(i);
        edge_id_to_span_count.insert({cur_id_to_span_count_pair.edge_id(), cur_id_to_span_count_pair.span_count()});
    }

    return {tc_, router_settings_, graph_.value(),
            DeserializeRouter(read_db_and_settings.transport_router().internal_data(), graph_.value()),
            edge_id_to_bus_name, edge_id_to_span_count
    };

}

db_serialization::RenderSettings TransportInformator::Serialize::Serializator::SerializeRenderSettings(
        const TransportInformator::Render::RenderSettings &render_settings) {
    db_serialization::RenderSettings result;

    result.set_width(render_settings.width);
    result.set_height(render_settings.height);
    result.set_padding(render_settings.padding);
    result.set_line_width(render_settings.line_width);
    result.set_stop_radius(render_settings.stop_radius);
    result.set_bus_label_font_size(render_settings.bus_label_font_size);
    result.set_bus_label_offset_x(render_settings.bus_label_offset[0]);
    result.set_bus_label_offset_y(render_settings.bus_label_offset[1]);
    result.set_stop_label_font_size(render_settings.stop_label_font_size);
    result.set_stop_label_offset_x(render_settings.stop_label_offset[0]);
    result.set_stop_label_offset_y(render_settings.stop_label_offset[1]);


    db_serialization::Color* underlayer_color_ptr = result.mutable_underlayer_color();
    *underlayer_color_ptr = SerializeColor(render_settings.underlayer_color);

    result.set_underlayer_width(render_settings.underlayer_width);

    for (const auto& color : render_settings.color_palette)
    {
        db_serialization::Color* cur_color_ptr = result.add_color_palette();
        *cur_color_ptr = SerializeColor(color);
    }



    return result;
}

TransportInformator::Render::RenderSettings TransportInformator::Serialize::Serializator::DeserializeRenderSettings
(const db_serialization::RenderSettings& render_settings) {
    TransportInformator::Render::RenderSettings result;

    result.width = render_settings.width();
    result.height = render_settings.height();
    result.padding = render_settings.padding();
    result.line_width = render_settings.line_width();
    result.stop_radius = render_settings.stop_radius();
    result.bus_label_font_size = render_settings.bus_label_font_size();
    result.bus_label_offset[0]  = render_settings.bus_label_offset_x();
    result.bus_label_offset[1] = render_settings.bus_label_offset_y();
    result.stop_label_font_size = render_settings.stop_label_font_size();
    result.stop_label_offset[0] = render_settings.stop_label_offset_x();
    result.stop_label_offset[1] = render_settings.stop_label_offset_y();
    result.underlayer_color = DeserializeColor(render_settings.underlayer_color());
    result.underlayer_width = render_settings.underlayer_width();
    for (int i = 0; i < render_settings.color_palette_size(); ++i)
    {
        result.color_palette.push_back(DeserializeColor(render_settings.color_palette(i)));
    }


    return result;
}

TransportInformator::Render::RenderSettings TransportInformator::Serialize::Serializator::GetRenderSettings() const {
    return render_setings_;
}

db_serialization::Color TransportInformator::Serialize::Serializator::SerializeColor(const svg::Color &color) {
    db_serialization::Color result;

    if (std::holds_alternative<svg::Rgb>(color))
    {
        auto color_rgb_ptr = result.mutable_color_rgb();
        color_rgb_ptr->set_red(std::get<svg::Rgb>(color).red);
        color_rgb_ptr->set_blue(std::get<svg::Rgb>(color).blue);
        color_rgb_ptr->set_green(std::get<svg::Rgb>(color).green);
    }

    if (std::holds_alternative<svg::Rgba>(color))
    {
        auto color_rgba_ptr = result.mutable_color_rgba();
        color_rgba_ptr->set_red(std::get<svg::Rgba>(color).red);
        color_rgba_ptr->set_blue(std::get<svg::Rgba>(color).blue);
        color_rgba_ptr->set_green(std::get<svg::Rgba>(color).green);
        color_rgba_ptr->set_alpha(std::get<svg::Rgba>(color).opacity);
    }

    if (std::holds_alternative<std::string>(color))
    {
        auto color_string_ptr = result.mutable_color_string();
        color_string_ptr->set_color_string(std::get<std::string>(color));
    }

    return result;
}

svg::Color TransportInformator::Serialize::Serializator::DeserializeColor(const db_serialization::Color& s_color) {
    if (s_color.has_color_rgb())
    {
        return svg::Rgb{static_cast<uint8_t>(s_color.color_rgb().red()),
                static_cast<uint8_t>(s_color.color_rgb().green()),
                static_cast<uint8_t>(s_color.color_rgb().blue()) };
    }

    if (s_color.has_color_rgba())
    {
        return svg::Rgba{static_cast<uint8_t>(s_color.color_rgba().red()),
                        static_cast<uint8_t>(s_color.color_rgba().green()),
                        static_cast<uint8_t>(s_color.color_rgba().blue()),
                        s_color.color_rgba().alpha()};
    }

    if (s_color.has_color_string())
    {
        return s_color.color_string().color_string();
    }

    return {};
}

db_serialization::TransportRouterParameters TransportInformator::Serialize::Serializator::SerializeRouterSettings(
        const TransportInformator::Router::TransportRouterParameters &router_params) {
    db_serialization::TransportRouterParameters result;
    result.set_bus_velocity(router_params.GetBusVelocity());
    result.set_bus_wait_time(router_params.GetBusWaitTime());
    return result;
}

TransportInformator::Router::TransportRouterParameters
TransportInformator::Serialize::Serializator::DeserializeRouterSettings(
        const db_serialization::TransportRouterParameters &router_settings) {
    TransportInformator::Router::TransportRouterParameters result;
    result.SetBusVelocity(router_settings.bus_velocity());
    result.SetBusWaitTime(router_settings.bus_wait_time());
    return result;
}

TransportInformator::Router::TransportRouterParameters
TransportInformator::Serialize::Serializator::GetRouterSettings() const {
    return router_settings_;
}

db_serialization::Graph
TransportInformator::Serialize::Serializator::SerializeGraph(const graph::DirectedWeightedGraph<double> &graph) {
    db_serialization::Graph result;

    auto edges = graph.GetAllEdges();

    for (const auto& edge : edges)
    {
        auto cur_edge_ptr = result.add_edges();
        cur_edge_ptr->set_from(edge.from);
        cur_edge_ptr->set_to(edge.to);
        cur_edge_ptr->set_weight(edge.weight);
    }

    auto incidence_lists = graph.GetAllIncidenceLists();

    for (const auto& incidence_list : incidence_lists)
    {
        auto cur_incidence_list = result.add_incidence_lists();
        for (const auto& edge_id : incidence_list)
        {
            cur_incidence_list->add_edge_id(edge_id);
        }
    }

    return result;
}

graph::DirectedWeightedGraph<double>
TransportInformator::Serialize::Serializator::DeserializeGraph(const db_serialization::Graph &graph) {

    std::vector<graph::Edge<double>> edges;
    int edges_size = graph.edges_size();
    for (int i = 0; i < edges_size; ++i)
    {
        edges.push_back({graph.edges(i).from(), graph.edges(i).to(), graph.edges(i).weight()});
    }

    std::vector<graph::DirectedWeightedGraph<double>::IncidenceList> incidence_lists;
    int incidence_lists_size = graph.incidence_lists_size();
    for (int i = 0; i < incidence_lists_size; ++i)
    {
        auto current_incidence_list = graph.incidence_lists(i);

        std::vector<size_t> unserialized_incidence_list;
        for (int j = 0; j < current_incidence_list.edge_id_size(); ++j)
        {
            unserialized_incidence_list.push_back(current_incidence_list.edge_id(j));
        }
        incidence_lists.push_back(unserialized_incidence_list);
    }

    return {edges, incidence_lists};

}

db_serialization::RoutesInternalData
TransportInformator::Serialize::Serializator::SerializeRouter(const graph::Router<double> &router) {
    db_serialization::RoutesInternalData result;

    auto routes_internal = router.GetRoutesInternalData();
    for (const auto& internal_data : routes_internal)
    {
        auto cur_internal_data  = result.add_all_router_data();
        for (const auto& data : internal_data)
        {
            auto cur_rid_vector = cur_internal_data->add_rid_vector();
            if (data.has_value())
            {
                if (data.value().prev_edge.has_value())
                {
                    cur_rid_vector->mutable_prev_edge_wrap()->set_prev_edge(data.value().prev_edge.value());
                    cur_rid_vector->set_weight(data.value().weight);
                }
            }
            else {
                cur_rid_vector->set_weight(-1);
            }
        }
    }
    return result;
}

graph::Router<double> TransportInformator::Serialize::Serializator::DeserializeRouter(
        const db_serialization::RoutesInternalData &router_data,
        const graph::DirectedWeightedGraph<double>& graph) {

    std::vector<std::vector<std::optional<graph::Router<double>::RouteInternalData>>> result;
    int router_data_size = router_data.all_router_data_size();
    for (int i = 0; i < router_data_size; ++i)
    {
        auto cur_router_data = router_data.all_router_data(i);
        std::vector<std::optional<graph::Router<double>::RouteInternalData>> vec_of_route_internal;
        for (int j = 0; j < cur_router_data.rid_vector_size(); ++j)
        {
            auto route_internal = cur_router_data.rid_vector(j);

            if (route_internal.has_prev_edge_wrap())
            {
                vec_of_route_internal.push_back(graph::Router<double>::RouteInternalData
                {route_internal.weight(), route_internal.prev_edge_wrap().prev_edge()});
            } else {
                vec_of_route_internal.push_back(
                        graph::Router<double>::RouteInternalData{route_internal.weight(), std::nullopt});
            }



        }
        result.push_back(vec_of_route_internal);
    }

    return {graph, result};
}
