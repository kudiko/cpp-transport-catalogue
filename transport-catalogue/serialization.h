#include <string>
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "transport_router.pb.h"

namespace TransportInformator {
    namespace Serialize
    {
        struct SerializationParameters {
            std::string file;
        };


        class Serializator
        {
        public:
            Serializator(Core::TransportCatalogue& tc, SerializationParameters pars);

            void SerializeToFile(const TransportInformator::Render::RenderSettings& render_settings,
                                 const TransportInformator::Router::TransportRouterParameters& router_parameters,
                                 const TransportInformator::Router::TransportRouter& transport_router);
            TransportInformator::Router::TransportRouter UnserializeFromFile();

            TransportInformator::Render::RenderSettings GetRenderSettings() const;
            TransportInformator::Router::TransportRouterParameters GetRouterSettings() const;
            graph::DirectedWeightedGraph<double> GetGraph() const
            {
                assert(graph_.has_value());
                return graph_.value();
            }


        private:
            Core::TransportCatalogue& tc_;
            SerializationParameters pars_;



            TransportInformator::Render::RenderSettings render_setings_;
            TransportInformator::Router::TransportRouterParameters router_settings_;
            std::optional<graph::DirectedWeightedGraph<double>> graph_;

            static db_serialization::RenderSettings SerializeRenderSettings(const TransportInformator::Render::RenderSettings& render_settings);
            static TransportInformator::Render::RenderSettings DeserializeRenderSettings(const db_serialization::RenderSettings& render_settings);
            static db_serialization::Color SerializeColor(const svg::Color& color);
            static svg::Color DeserializeColor(const db_serialization::Color& s_color);
            static db_serialization::TransportRouterParameters SerializeRouterSettings(const TransportInformator::Router::TransportRouterParameters& router_params);
            static TransportInformator::Router::TransportRouterParameters DeserializeRouterSettings(const db_serialization::TransportRouterParameters& router_settings);
            static db_serialization::Graph SerializeGraph(const graph::DirectedWeightedGraph<double>& graph);
            static graph::DirectedWeightedGraph<double> DeserializeGraph(const db_serialization::Graph& graph);
            static db_serialization::RoutesInternalData SerializeRouter(const graph::Router<double>& router);
            static graph::Router<double> DeserializeRouter(const db_serialization::RoutesInternalData& router_data, const graph::DirectedWeightedGraph<double>& graph);

        };
    }


}