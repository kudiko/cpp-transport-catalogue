#include "json_reader.h"
#include <cassert>
#include <algorithm>
#include "map_renderer.h"
/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace TransportInformator
{

    namespace Input
    {

        BaseRequest::BaseRequest(BaseRequestType new_type, const std::string &new_name) : type{new_type}, name{new_name} {}

        AddStopRequest::AddStopRequest(BaseRequestType new_type, const std::string &new_name,
                                       detail::Coordinates new_coords, const std::vector<std::pair<std::string, double>> &distances) : BaseRequest{new_type, new_name}, coords{new_coords}, distances_to_other_stops{std::move(distances)} {}

        AddBusRequest::AddBusRequest(BaseRequestType new_type, const std::string &new_name, const std::vector<std::string> &new_stops, bool isroundtrip) 
        : BaseRequest{new_type, new_name}, stops{std::move(new_stops)}, is_roundtrip{isroundtrip} {}

        StatRequest::StatRequest(size_t new_id, StatRequestType new_type) : id{new_id}, type{new_type}{}
        BusInfoRequest::BusInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name) : StatRequest{new_id, new_type}, name{new_name}{}
        StopInfoRequest::StopInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name) : StatRequest{new_id, new_type}, name{new_name}{}
        MapRenderRequest::MapRenderRequest(size_t new_id, StatRequestType new_type) : StatRequest{new_id, new_type}{}

        void BusInfoRequest::Process(JSONReader& jreader, [[maybe_unused]] ReqHandler::RequestHandler& rh)
        {
            using namespace std::literals;
            auto info = jreader.tc_.GetBusInfo(name);
            if (!info.has_value())
            {
                json::Dict dict{{"request_id", json::Node{static_cast<int>(id)}}, {"error_message", {"not found"s}}};

                jreader.db_answers_.push_back(std::move(json::Node{dict}));
                return;
            }

            json::Dict dict{{"request_id", json::Node{static_cast<int>(id)}}, {"route_length", {info->route_length}}, {"curvature", {info->curvature}}, {"stop_count", {static_cast<int>(info->stops)}}, {"unique_stop_count", {static_cast<int>(info->unique_stops)}}};
            jreader.db_answers_.push_back(std::move(json::Node{dict}));
        }

        void StopInfoRequest::Process(JSONReader& jreader, [[maybe_unused]] ReqHandler::RequestHandler& rh)
        {
            using namespace std::literals;
            auto info = jreader.tc_.GetStopInfo(name);
            if (!info.has_value())
            {
                json::Dict dict{{"request_id", json::Node{static_cast<int>(id)}}, {"error_message", {"not found"s}}};

                jreader.db_answers_.push_back(std::move(json::Node{dict}));
                return;
            }

            json::Array arr;
            for (std::string_view bus_stop : info->buses)
            {
                arr.push_back({static_cast<std::string>(bus_stop)});
            }

            json::Dict dict{{"request_id", json::Node{static_cast<int>(id)}}, {"buses", {arr}}};

            jreader.db_answers_.push_back(std::move(json::Node{dict}));
        }

        void MapRenderRequest::Process(JSONReader& jreader, ReqHandler::RequestHandler& rh)
        {
            std::stringstream ss;
            rh.RenderMap().Render(ss);
            
            json::Dict dict{{"request_id", json::Node{static_cast<int>(id)} }, {"map", {ss.str()}}};
            jreader.db_answers_.push_back(std::move(json::Node{dict}));
        }


        JSONReader::JSONReader(Core::TransportCatalogue &tc, std::istream &in) : tc_{tc}, in_{in}{}

        void JSONReader::Print(std::ostream &out)
        {
            json::Document out_doc{json::Node{db_answers_}};
            json::Print(out_doc, out);
        }

        void JSONReader::SendBaseRequests()
        {
            for (const AddStopRequest &stop_command : commands_.base_requests.add_stop)
            {
                tc_.AddStop(stop_command.name, stop_command.coords);
            }

            for (const AddStopRequest &stop_command : commands_.base_requests.add_stop)
            {
                SetDistancesToOtherStops(stop_command.name, stop_command.distances_to_other_stops);
            }

            for (const AddBusRequest &bus_command : commands_.base_requests.add_bus)
            {
                tc_.AddBus(bus_command.name, bus_command.stops, bus_command.is_roundtrip);
            }
        }

        void JSONReader::SendStatRequests(ReqHandler::RequestHandler& rh)
        {
            for (const auto& req : stat_requests_)
            {
                req->Process(*this, rh);
            }

            Print(std::cout);
        }

        void JSONReader::ReadJSON()
        {
            json::Node root_node = json::Load(in_).GetRoot();
            assert(root_node.IsMap());
            json::Dict map = root_node.AsMap();

            assert(map.count("base_requests"));
            assert(map.at("base_requests").IsArray());
            json::Array base_requests = map.at("base_requests").AsArray();
            ProcessBaseRequests(base_requests);
            SendBaseRequests();

            assert(map.count("render_settings"));
            assert(map.at("render_settings").IsMap());
            json::Dict render_settings = map.at("render_settings").AsMap();
            ProcessRenderSettings(render_settings);

            
            assert(map.count("stat_requests"));
            assert(map.at("stat_requests").IsArray());
            json::Array stat_requests = map.at("stat_requests").AsArray();
            AddStatRequests(stat_requests);

        }

        void JSONReader::ProcessBaseRequests(const json::Array &arr)
        {
            for (const auto &command : arr)
            {
                assert(command.IsMap());
                json::Dict m = command.AsMap();
                if (m.at("type").AsString() == "Stop")
                {
                    ProcessAddStop(m);
                }
                else if (m.at("type").AsString() == "Bus")
                {
                    ProcessAddBus(m);
                }
                else
                {
                    throw std::logic_error("Wrong base request");
                }
            }
        }

        void JSONReader::AddStatRequests(const json::Array &arr)
        {
            for (const auto &command : arr)
            {
                assert(command.IsMap());
                json::Dict m = command.AsMap();
                if (m.at("type").AsString() == "Stop")
                {
                    StopInfoRequest stop_info_r{static_cast<size_t>(m.at("id").AsInt()),StatRequestType::STOP, m.at("name").AsString()};
                    stat_requests_.push_back(std::make_unique<StopInfoRequest>(stop_info_r));
                }
                else if (m.at("type").AsString() == "Bus")
                {
                    BusInfoRequest bus_info_r {static_cast<size_t>(m.at("id").AsInt()), StatRequestType::BUS, m.at("name").AsString()};
                    stat_requests_.push_back(std::make_unique<BusInfoRequest>(bus_info_r));
                }
                else if (m.at("type").AsString() == "Map")
                {
                    MapRenderRequest map_render_r {static_cast<size_t>(m.at("id").AsInt()), StatRequestType::MAP};
                    stat_requests_.push_back(std::make_unique<MapRenderRequest>(map_render_r));
                }
                else
                {
                    throw std::logic_error("Wrong stat request");
                }
            }
        }

        void JSONReader::ProcessAddStop(const json::Dict &add_stop_command)
        {
            std::vector<std::pair<std::string, double>> distances_to_other_stops;
            assert(add_stop_command.at("road_distances").IsMap());
            json::Dict distances = add_stop_command.at("road_distances").AsMap();
            for (const auto &[other_stop, distance] : distances)
            {
                distances_to_other_stops.push_back({other_stop, distance.AsDouble()});
            }

            AddStopRequest new_add_stop_request{BaseRequestType::STOP, add_stop_command.at("name").AsString(), {add_stop_command.at("latitude").AsDouble(), add_stop_command.at("longitude").AsDouble()}, distances_to_other_stops};

            commands_.base_requests.add_stop.push_back(std::move(new_add_stop_request));
        }

        void JSONReader::ProcessAddBus(const json::Dict &add_bus_command)
        {
            std::vector<std::string> stops;
            assert(add_bus_command.at("stops").IsArray());
            for (const auto &stop_name : add_bus_command.at("stops").AsArray())
            {
                assert(stop_name.IsString());
                stops.push_back(stop_name.AsString());
            }

            bool is_roundtrip = add_bus_command.at("is_roundtrip").AsBool();

            AddBusRequest new_add_bus_request{BaseRequestType::BUS, add_bus_command.at("name").AsString(), std::move(stops), is_roundtrip};
            commands_.base_requests.add_bus.push_back(std::move(new_add_bus_request));
        }


        void JSONReader::SetDistancesToOtherStops(std::string_view start, const std::vector<std::pair<std::string, double>> &distances_to_other_stops)
        {
            for (const auto &[other_stop_name, distance] : distances_to_other_stops)
            {
                if (!tc_.FindStop(other_stop_name))
                {
                    tc_.AddStop(other_stop_name, {}); // adding stop to be filled in the future
                }
                tc_.SetDistanceBetweenStops(tc_.FindStop(start), tc_.FindStop(other_stop_name), distance);
            }
        }

        void JSONReader::ProcessRenderSettings(const json::Dict& dict)
        {
            render_settings_.width = dict.at("width").AsDouble();
            render_settings_.height = dict.at("height").AsDouble();
            render_settings_.padding = dict.at("padding").AsDouble();
            assert(render_settings_.padding >= 0 && render_settings_.padding < std::min(render_settings_.width, render_settings_.height) / 2);
            render_settings_.line_width = dict.at("line_width").AsDouble();
            render_settings_.stop_radius = dict.at("stop_radius").AsDouble();
            render_settings_.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
            render_settings_.bus_label_offset[0] = dict.at("bus_label_offset").AsArray().at(0).AsDouble();
            render_settings_.bus_label_offset[1] = dict.at("bus_label_offset").AsArray().at(1).AsDouble();
            render_settings_.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
            render_settings_.stop_label_offset[0] = dict.at("stop_label_offset").AsArray().at(0).AsDouble();
            render_settings_.stop_label_offset[1] = dict.at("stop_label_offset").AsArray().at(1).AsDouble();
            
            render_settings_.underlayer_color = ParseColorFromJSON(dict.at("underlayer_color"));
            render_settings_.underlayer_width = dict.at("underlayer_width").AsDouble();

            json::Array color_p = dict.at("color_palette").AsArray();
            for (const auto& node : color_p)
            {
                render_settings_.color_palette.push_back(ParseColorFromJSON(node));
            }

        }

        svg::Color JSONReader::ParseColorFromJSON(const json::Node& node) const
        {
            if (node.IsString())
            {
                return node.AsString();
            }
            if (node.IsArray())
            {
                json::Array arr = node.AsArray();
                if (arr.size() == 3)
                {
                    return svg::Rgb{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), 
                    static_cast<uint8_t>(arr[2].AsInt())};
                }
                else if (arr.size() == 4)
                {
                    return svg::Rgba{static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), 
                    static_cast<uint8_t>(arr[2].AsInt()), arr[3].AsDouble()};
                } else {
                    throw std::logic_error("Wrong color array size");
                }
            }
            throw std::logic_error("Wrong color detected when parsing");
        }

        Render::RenderSettings JSONReader::GetRenderSettings() const
        {
            return render_settings_;
        }

    } // namespace Input

}