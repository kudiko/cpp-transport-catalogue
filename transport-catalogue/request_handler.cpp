#include "request_handler.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include <sstream>
#include <algorithm>
#include <iomanip>




namespace TransportInformator
{

    namespace ReqHandler
    {

        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler::RequestHandler(const Core::TransportCatalogue &db, Render::MapRenderer &renderer) : db_{db}, renderer_{renderer}{}

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<Core::BusInfo> RequestHandler::GetBusStat(const std::string_view &bus_name) const
        {
            return db_.GetBusInfo(bus_name);
        }

        // Возвращает маршруты, проходящие через остановку
        const std::set<std::string_view> RequestHandler::GetBusesByStop(const std::string_view &stop_name) const
        {
            return db_.GetBusesForStop(stop_name);
        }

        // Этот метод будет нужен в следующей части итогового проекта
        const svg::Document& RequestHandler::RenderMap()
        {
            const auto buses_to_draw = db_.GetAllNonEmptyBuses();
            for (const auto& bus_name : buses_to_draw)
            {
                const Core::Bus* info = db_.FindBus(bus_name);
                renderer_.DrawBus({info->name, db_.GetBusStopCoordsForBus(bus_name), info->is_roundtrip});
            }

            for (const auto& bus_name : buses_to_draw)
            {
                const auto start = db_.FindBus(bus_name)->stops.front();
                const auto finish = FindLastStop(bus_name);

                renderer_.DrawBusLabel({bus_name, start->coords, finish->coords});
            }

            const auto stops_to_draw = db_.GetAllNonEmptyStops();

            for (const auto& stop_name : stops_to_draw)
            {
                renderer_.DrawStopSymbol(db_.FindStop(stop_name));
            }

            for (const auto& stop_name : stops_to_draw)
            {
                renderer_.DrawStopLabel(db_.FindStop(stop_name));
            }



            return renderer_.GetDocument();
            
        }

        const Core::Stop* RequestHandler::FindLastStop(std::string_view bus_name) const
        {
            std::vector<const Core::Stop*>& stop_vec_ref = db_.FindBus(bus_name)->stops;

            bool is_circular = false;
            for (std::size_t i = 0; i < stop_vec_ref.size() / 2; ++i)
            {
                if (stop_vec_ref[0] != stop_vec_ref[stop_vec_ref.size() - 1 - i])
                {
                    is_circular = true;
                }
            }

            if (is_circular)
            {
                return stop_vec_ref.back();
            } else {
                return stop_vec_ref[stop_vec_ref.size() / 2];
            }
        }
    }

namespace Input
{

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

    InputReader::AddStopCommand InputReader::ParseStopCommand(std::stringstream& ss)
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

    InputReader::AddBusCommand InputReader::ParseBusCommand(std::stringstream& ss)
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
} //namespace TransportInformator::Input

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

    void StatReader::PrintBusResult(std::stringstream& ss, std::ostream &out)
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
    
    void StatReader::PrintStopResult(std::stringstream& ss, std::ostream &out)
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

} // namespace TransportInformator