#include "stat_reader.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace TransportInformator
{

namespace Output
{

    StatReader::StatReader(const Core::TransportCatalogue &tc) : tc_(tc) {}

    void StatReader::ReadInput()
    {
        ReadInput(std::cin, std::cout);
    }

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

    void StatReader::ProcessCommand(std::string_view command)
    {
        ProcessCommand(command, std::cout);
    }

    void StatReader::ProcessCommand(std::string_view command, std::ostream &out)
    {
        std::stringstream ss(static_cast<std::string>(command));
        std::string name;

        ss >> name;
        if (name == "Bus")
        {
            ss.ignore(1);
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
        else if (name == "Stop")
        {
            ss.ignore(1);
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
        else
        {
            throw std::invalid_argument("Wrong command");
        }
    }
} // namespace TransportInformator::Output
} // namespace TransportInformator