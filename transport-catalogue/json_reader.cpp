    #include "json_reader.h"
    #include <algorithm>
    #include <iomanip>
    #include "map_renderer.h"
    #include "json_builder.h"
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

            StatRequest::StatRequest(size_t new_id, StatRequestType new_type) : id{new_id}, type{new_type} {}
            BusInfoRequest::BusInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name) : StatRequest{new_id, new_type}, name{new_name} {}
            StopInfoRequest::StopInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name) : StatRequest{new_id, new_type}, name{new_name} {}
            MapRenderRequest::MapRenderRequest(size_t new_id, StatRequestType new_type) : StatRequest{new_id, new_type} {}
            RouteRequest::RouteRequest(size_t new_id, StatRequestType new_type, std::string name_from, std::string name_to) :
            StatRequest{new_id, new_type}, from{move(name_from)}, to{move(name_to)} {}


            json::Node BusInfoRequest::Process(JSONReader &jreader, [[maybe_unused]] ReqHandler::RequestHandler &rh)
            {
                using namespace std::literals;
                auto info = jreader.tc_.GetBusInfo(name);

                if (!info.has_value())
                {
                    return json::Builder{}
                    .StartDict()
                        .Key("request_id").ValueInDictItem(static_cast<int>(id))
                        .Key("error_message").ValueInDictItem("not found"s)
                    .EndDict()
                    .Build();
                }

                return json::Builder{}.StartDict()
                    .Key("request_id").ValueInDictItem(static_cast<int>(id))
                    .Key("route_length").ValueInDictItem(info->route_length)
                    .Key("curvature").ValueInDictItem(info->curvature)
                    .Key("stop_count").ValueInDictItem(static_cast<int>(info->stops))
                    .Key("unique_stop_count").ValueInDictItem(static_cast<int>(info->unique_stops))
                .EndDict()
                .Build();
            }

            json::Node StopInfoRequest::Process(JSONReader &jreader, [[maybe_unused]] ReqHandler::RequestHandler &rh)
            {
                using namespace std::literals;
                auto info = jreader.tc_.GetStopInfo(name);
                if (!info.has_value())
                {
                    return json::Builder{}
                    .StartDict()
                        .Key("request_id").ValueInDictItem(static_cast<int>(id))
                        .Key("error_message").ValueInDictItem("not found"s)
                    .EndDict()
                    .Build();
                }

                json::Array arr;
                for (std::string_view bus_stop : info->buses)
                {
                    arr.push_back({static_cast<std::string>(bus_stop)});
                }

                return json::Builder{}
                .StartDict()
                    .Key("request_id").ValueInDictItem(static_cast<int>(id))
                    .Key("buses").ValueInDictItem(arr)
                .EndDict()
                .Build();
            }

            json::Node MapRenderRequest::Process([[maybe_unused]] JSONReader &jreader, ReqHandler::RequestHandler &rh)
            {
                std::stringstream ss;
                rh.RenderMap().Render(ss);

                return json::Builder{}
                .StartDict()
                    .Key("request_id").ValueInDictItem(static_cast<int>(id))
                    .Key("map").ValueInDictItem(ss.str())
                .EndDict()
                .Build();
            }

            json::Node RouteRequest::Process([[maybe_unused]] JSONReader& jreader, ReqHandler::RequestHandler& rh)
            {
                using namespace std::literals;

                std::optional<Router::Route> built_route = rh.BuildRoute(from, to);
                if (!built_route.has_value() || built_route.value().total_time == -1)
                {
                    return json::Builder{}
                    .StartDict()
                        .Key("request_id").ValueInDictItem(static_cast<int>(id))
                        .Key("error_message").ValueInDictItem("not found"s)
                    .EndDict()
                    .Build();
                }

                json::Array items_arr;
                for (const auto& route_element : built_route.value().route_details)
                {
                    if (std::holds_alternative<Router::Route::RouteElementWait>(route_element))
                    {
                        json::Node wait_node = json::Builder{}
                        .StartDict()
                            .Key("type").ValueInDictItem("Wait"s)
                            .Key("stop_name").ValueInDictItem(std::get<Router::Route::RouteElementWait>(route_element).stop_name)
                            .Key("time").ValueInDictItem(std::get<Router::Route::RouteElementWait>(route_element).time)
                        .EndDict()
                        .Build();
                        items_arr.push_back(wait_node);
                        continue;
                    }
                    json::Node bus_node = json::Builder{}
                        .StartDict()
                            .Key("type").ValueInDictItem("Bus"s)
                            .Key("bus").ValueInDictItem(std::get<Router::Route::RouteElementBus>(route_element).bus)
                            .Key("span_count").ValueInDictItem(std::get<Router::Route::RouteElementBus>(route_element).span_count)
                            .Key("time").ValueInDictItem(std::get<Router::Route::RouteElementBus>(route_element).time)
                        .EndDict()
                        .Build();
                    items_arr.push_back(bus_node);
                }

                return json::Builder{}
                    .StartDict()
                        .Key("request_id").ValueInDictItem(static_cast<int>(id))
                        .Key("total_time").ValueInDictItem(built_route.value().total_time)
                        .Key("items").ValueInDictItem(items_arr)
                    .EndDict()
                    .Build();
            }

            JSONReader::JSONReader(Core::TransportCatalogue &tc, std::istream &in) : tc_{tc}, in_{in} {}

            void JSONReader::Print(std::ostream &out, json::Document doc_to_print)
            {
                json::Print(doc_to_print, out);
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

            void JSONReader::SendStatRequests(ReqHandler::RequestHandler &rh)
            {
                json::Builder builder{};
                builder.StartArray();

                for (const auto &req : stat_requests_)
                {
                    builder.Value(req->Process(*this, rh).AsDict());
                }

                builder.EndArray();

                Print(std::cout, json::Document{builder.Build()});
            }

            void JSONReader::ReadMakeBaseJSON()
            {
                json::Node root_node = json::Load(in_).GetRoot();
                if (!root_node.IsDict())
                {
                    throw std::invalid_argument("Parent node of JSON is not map");
                }
                json::Dict map = root_node.AsDict();

                if (!map.count("base_requests"))
                {
                    throw std::invalid_argument("There is no key \"base_requests\" in JSON");
                }
                if (!map.at("base_requests").IsArray())
                {
                    throw std::invalid_argument("Key \"base_requests\" is not array in JSON");
                }
                json::Array base_requests = map.at("base_requests").AsArray();
                ProcessBaseRequests(base_requests);
                SendBaseRequests();


                if (!map.count("render_settings"))
                {
                    throw std::invalid_argument("There is no key \"render_settings\" in JSON");
                }
                if (!map.at("render_settings").IsDict())
                {
                    throw std::invalid_argument("Key \"render_settings\" is not map in JSON");
                }
                json::Dict render_settings = map.at("render_settings").AsDict();
                ProcessRenderSettings(render_settings);

                if (!map.count("routing_settings"))
                {
                    throw std::invalid_argument("There is no key \"routing_settings\" in JSON");
                }
                if (!map.at("routing_settings").IsDict())
                {
                    throw std::invalid_argument("Key \"routing_settings\" is not map in JSON");
                }
                json::Dict router_settings = map.at("routing_settings").AsDict();
                ProcessRouterSettings(router_settings);

                if (!map.count("serialization_settings"))
                {
                    throw std::invalid_argument("There is no key \"serialization_settings\" in JSON");
                }
                if (!map.at("serialization_settings").IsDict())
                {
                    throw std::invalid_argument("Key \"serialization_settings\" is not map in JSON");
                }
                json::Dict serialization_settings = map.at("serialization_settings").AsDict();
                ProcessSerializationSettings(serialization_settings);
            }

            void JSONReader::ReadProcessRequestsJSON()
            {
                json::Node root_node = json::Load(in_).GetRoot();

                if (!root_node.IsDict())
                {
                    throw std::invalid_argument("Parent node of JSON is not map");
                }
                json::Dict map = root_node.AsDict();

                if (!map.count("stat_requests"))
                {
                    throw std::invalid_argument("There is no key \"stat_requests\" in JSON");
                }
                if (!map.at("stat_requests").IsArray())
                {
                    throw std::invalid_argument("Key \"stat_requests\" is not array in JSON");
                }
                json::Array stat_requests = map.at("stat_requests").AsArray();
                AddStatRequests(stat_requests);

                if (!map.count("serialization_settings"))
                {
                    throw std::invalid_argument("There is no key \"serialization_settings\" in JSON");
                }
                if (!map.at("serialization_settings").IsDict())
                {
                    throw std::invalid_argument("Key \"serialization_settings\" is not map in JSON");
                }
                json::Dict serialization_settings = map.at("serialization_settings").AsDict();
                ProcessSerializationSettings(serialization_settings);
            }

            void JSONReader::ProcessBaseRequests(const json::Array &arr)
            {
                for (const auto &command : arr)
                {
                    if (!command.IsDict())
                    {
                        throw std::invalid_argument("Base request in JSON is not map");
                    }
                    json::Dict current_command = command.AsDict();
                    if (current_command.at("type").AsString() == "Stop")
                    {
                        ProcessAddStop(current_command);
                    }
                    else if (current_command.at("type").AsString() == "Bus")
                    {
                        ProcessAddBus(current_command);
                    }
                    else
                    {
                        throw std::invalid_argument("Wrong base request");
                    }
                }
            }

            void JSONReader::AddStatRequests(const json::Array &arr)
            {
                for (const auto &command : arr)
                {
                    if (!command.IsDict())
                    {
                        throw std::invalid_argument("Stat request in JSON is not map");
                    }
                    json::Dict current_request = command.AsDict();
                    if (current_request.at("type").AsString() == "Stop")
                    {
                        StopInfoRequest stop_info_r{static_cast<size_t>(current_request.at("id").AsInt()),
                                                    StatRequestType::STOP, current_request.at("name").AsString()};
                        stat_requests_.push_back(std::make_unique<StopInfoRequest>(stop_info_r));
                    }
                    else if (current_request.at("type").AsString() == "Bus")
                    {
                        BusInfoRequest bus_info_r{static_cast<size_t>(current_request.at("id").AsInt()),
                                                  StatRequestType::BUS, current_request.at("name").AsString()};
                        stat_requests_.push_back(std::make_unique<BusInfoRequest>(bus_info_r));
                    }
                    else if (current_request.at("type").AsString() == "Map")
                    {
                        MapRenderRequest map_render_r{static_cast<size_t>(current_request.at("id").AsInt()), StatRequestType::MAP};
                        stat_requests_.push_back(std::make_unique<MapRenderRequest>(map_render_r));
                    }
                    else if (current_request.at("type").AsString() == "Route")
                    {
                        RouteRequest route_r{static_cast<size_t>(current_request.at("id").AsInt()), StatRequestType::ROUTE,
                        current_request.at("from").AsString(), current_request.at("to").AsString()};
                        stat_requests_.push_back(std::make_unique<RouteRequest>(route_r));
                    }
                    else
                    {
                        throw std::invalid_argument("Wrong stat request");
                    }
                }
            }

            void JSONReader::ProcessAddStop(const json::Dict &add_stop_command)
            {
                std::vector<std::pair<std::string, double>> distances_to_other_stops;
                if (!add_stop_command.at("road_distances").IsDict())
                {
                    throw std::invalid_argument("Add stop request is not map");
                }
                json::Dict distances = add_stop_command.at("road_distances").AsDict();
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
                if (!add_bus_command.at("stops").IsArray())
                {
                    throw std::invalid_argument("Add bus request is not map");
                }
                for (const auto &stop_name : add_bus_command.at("stops").AsArray())
                {
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

            void JSONReader::ProcessRenderSettings(const json::Dict &dict)
            {
                render_settings_.width = dict.at("width").AsDouble();
                render_settings_.height = dict.at("height").AsDouble();
                render_settings_.padding = dict.at("padding").AsDouble();
                if (!(render_settings_.padding >= 0 && render_settings_.padding < std::min(render_settings_.width, render_settings_.height) / 2))
                {
                    throw std::invalid_argument("Wrong value of padding in render settings");
                }
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
                for (const auto &node : color_p)
                {
                    render_settings_.color_palette.push_back(ParseColorFromJSON(node));
                }
            }

            void JSONReader::ProcessRouterSettings(const json::Dict& dict)
            {
                router_settings_.SetBusWaitTime(dict.at("bus_wait_time").AsInt());
                router_settings_.SetBusVelocity(dict.at("bus_velocity").AsDouble());
            }

            void JSONReader::ProcessSerializationSettings(const json::Dict& dict)
            {
                serializatoin_settings_.file = dict.at("file").AsString();
            }

            svg::Color JSONReader::ParseColorFromJSON(const json::Node &node) const
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
                    }
                    else
                    {
                        throw std::logic_error("Wrong color array size");
                    }
                }
                throw std::logic_error("Wrong color detected when parsing");
            }

            Render::RenderSettings JSONReader::GetRenderSettings() const
            {
                return render_settings_;
            }
            Router::TransportRouterParameters JSONReader::GetRouterSettings() const
            {
                return router_settings_;
            }
            Serialize::SerializationParameters JSONReader::GetSerializationSettings() const
            {
                return serializatoin_settings_;
            }

            InputReader::InputReader(Core::TransportCatalogue &tc) : tc_(tc) {}

            void InputReader::ReadInput(std::istream &stream)
            {
                size_t N;
                stream >> N;

                stream.ignore(1);

                for (size_t i = 0; i < N; ++i)
                {
                    std::string line;
                    if (!std::getline(stream, line))
                    {
                        break;
                    }
                    ProcessCommand(line);
                }
                DeliverCommands();
            }

            void InputReader::DeliverCommands()
            {
                for (const AddStopCommand &stop_command : queue_.stop_commands)
                {
                    tc_.AddStop(stop_command.name, stop_command.coords);
                }

                for (const AddStopCommand &stop_command : queue_.stop_commands)
                {
                    SetDistancesToOtherStops(stop_command.name, stop_command.distances_to_other_stops);
                }

                for (const AddBusCommand &bus_command : queue_.bus_commands)
                {
                    tc_.AddBus(std::move(bus_command.name), std::move(bus_command.stops), bus_command.is_roundtrip);
                }
            }

            void InputReader::ProcessCommand(std::string_view command)
            {
                std::stringstream ss(static_cast<std::string>(command));
                std::string command_type;
                ss >> command_type;
                if (command_type == "Stop")
                {
                    queue_.stop_commands.push_back(std::move(ParseStopCommand(ss)));
                }
                else if (command_type == "Bus")
                {
                    queue_.bus_commands.push_back(std::move(ParseBusCommand(ss)));
                }
                else
                {
                    throw std::invalid_argument("Wrong command");
                }
            }

            InputReader::AddStopCommand InputReader::ParseStopCommand(std::stringstream &ss)
            {
                std::string name;
                ss.ignore(1);
                name.clear();
                while (ss.peek() != ':')
                {
                    name += ss.get();
                }

                ss.ignore(1);

                double lat;
                double lng;

                ss >> lat;
                ss.ignore(2);
                ss >> lng;
                ss.ignore(2);
                std::vector<std::pair<std::string, double>> distances_to_other_stops;

                std::string other_stop_entry;
                while (std::getline(ss, other_stop_entry, ','))
                {
                    std::stringstream other_stop_entry_ss;
                    other_stop_entry_ss << other_stop_entry; // 10.0m to stop#

                    std::string token;
                    other_stop_entry_ss >> token; // 10.0m
                    token.pop_back();             // 10.0
                    double distance = std::stod(token);

                    other_stop_entry_ss >> token; // to
                    other_stop_entry_ss.ignore(1);

                    std::string stop_name;
                    std::getline(other_stop_entry_ss, stop_name);

                    distances_to_other_stops.push_back({stop_name, distance});
                }

                return {name, {lat, lng}, distances_to_other_stops};
            }

            InputReader::AddBusCommand InputReader::ParseBusCommand(std::stringstream &ss)
            {
                std::string name;
                ss.ignore(1);
                name.clear();
                while (ss.peek() != ':')
                {
                    name += ss.get();
                }
                ss.ignore(1);

                bool is_circular = (ss.str().find('>') != std::string::npos) ? true : false;

                AddBusCommand new_bus_command{std::move(name), {}, is_circular};

                while (ss)
                {
                    std::string symbols;
                    ss.ignore(1);

                    while (!ss.eof() && ss.peek() != '>' && ss.peek() != '-')
                    {
                        symbols += ss.get();
                    }

                    symbols.pop_back();

                    new_bus_command.stops.push_back(std::move(symbols));
                    ss.ignore(1);
                }

                /*
                if (!is_circular)
                {
                    std::vector<std::string> reversed_stops(new_bus_command.stops.size() - 1);
                    std::reverse_copy(new_bus_command.stops.begin(), std::next(new_bus_command.stops.end(), -1), reversed_stops.begin());

                    std::vector<std::string> result(2 * new_bus_command.stops.size() - 1);
                    auto first_vector_end = std::move(new_bus_command.stops.begin(), new_bus_command.stops.end(), result.begin());
                    std::move(reversed_stops.begin(), reversed_stops.end(), first_vector_end);

                    std::swap(result, new_bus_command.stops);
                }*/

                return new_bus_command;
            }

            void InputReader::SetDistancesToOtherStops(std::string_view start, std::vector<std::pair<std::string, double>> distances_to_other_stops)
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
        } // namespace Input

        namespace Output
        {

            StatReader::StatReader(const Core::TransportCatalogue &tc) : tc_(tc) {}

            void StatReader::ReadInput(std::istream &stream, std::ostream &out)
            {
                size_t N;
                stream >> N;

                stream.ignore(1);

                for (size_t i = 0; i < N; ++i)
                {
                    std::string line;
                    if (!std::getline(stream, line))
                    {
                        break;
                    }
                    ProcessCommand(line, out);
                }
            }

            void StatReader::ProcessCommand(std::string_view command, std::ostream &out)
            {
                std::stringstream ss(static_cast<std::string>(command));
                std::string command_type;

                ss >> command_type;
                if (command_type == "Bus")
                {
                    PrintBusResult(ss, out);
                }
                else if (command_type == "Stop")
                {
                    PrintStopResult(ss, out);
                }
                else
                {
                    throw std::invalid_argument("Wrong command");
                }
            }

            void StatReader::PrintBusResult(std::stringstream &ss, std::ostream &out)
            {
                ss.ignore(1);
                std::string name;
                std::getline(ss, name);

                std::optional<Core::BusInfo> result = tc_.GetBusInfo(name);
                if (!result)
                {
                    out << "Bus " << name << ": not found" << std::endl;
                }
                else
                {
                    out << std::setprecision(6) << "Bus " << result->name << ": " << result->stops << " stops on route, " << result->unique_stops << " unique stops, " << result->route_length << " route length, " << result->curvature << " curvature" << std::endl;
                }
            }

            void StatReader::PrintStopResult(std::stringstream &ss, std::ostream &out)
            {
                ss.ignore(1);
                std::string name;
                std::getline(ss, name);
                std::optional<Core::StopInfo> result = tc_.GetStopInfo(name);

                if (!result)
                {
                    out << "Stop " << name << ": not found" << std::endl;
                    return;
                }

                if (!result->buses.size())
                {
                    out << "Stop " << name << ": no buses" << std::endl;
                    return;
                }

                out << "Stop " << name << ": buses";

                for (const auto bus_name : result->buses)
                {
                    out << " " << bus_name;
                }
                out << std::endl;
            }
        } // namespace TransportInformator::Output

    }