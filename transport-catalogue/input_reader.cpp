#include "input_reader.h"

#include <sstream>
#include <algorithm>

namespace TransportInformator
{

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
            tc_.AddBus(std::move(bus_command.name), std::move(bus_command.stops));
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

        AddBusCommand new_bus_command{std::move(name), {}};

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

        bool is_circular = (ss.str().find('>') != std::string::npos) ? true : false;

        if (!is_circular)
        {
            std::vector<std::string> reversed_stops(new_bus_command.stops.size() - 1);
            std::reverse_copy(new_bus_command.stops.begin(), std::next(new_bus_command.stops.end(), -1), reversed_stops.begin());

            std::vector<std::string> result(2 * new_bus_command.stops.size() - 1);
            auto first_vector_end = std::move(new_bus_command.stops.begin(), new_bus_command.stops.end(), result.begin());
            std::move(reversed_stops.begin(), reversed_stops.end(), first_vector_end);

            std::swap(result, new_bus_command.stops);
        }

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

} // namespace TransportInformator