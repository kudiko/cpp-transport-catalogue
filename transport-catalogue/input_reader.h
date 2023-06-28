#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <sstream>
#include <string>

namespace TransportInformator
{

namespace Input
{

    class InputReader
    {
    public:
        InputReader(Core::TransportCatalogue &tc);

        void ReadInput(std::istream &stream = std::cin);

        struct AddStopCommand
        {
            std::string name;
            detail::Coordinates coords;
            std::vector<std::pair<std::string, double>> distances_to_other_stops;
        };

        struct AddBusCommand
        {
            std::string name;
            std::vector<std::string> stops;
        };

        struct DBUpdateQueue
        {
            std::vector<AddStopCommand> stop_commands;
            std::vector<AddBusCommand> bus_commands;
        };

    private:
        void ProcessCommand(std::string_view command);
        AddStopCommand ParseStopCommand(std::stringstream& ss);
        AddBusCommand ParseBusCommand(std::stringstream& ss);
        

        void SetDistancesToOtherStops(std::string_view start, std::vector<std::pair<std::string, double>> distances_to_other_stops);
        
        void DeliverCommands();

        DBUpdateQueue queue_;
        Core::TransportCatalogue &tc_;
    };

} // namespace TransportInformator::Input

} // namespace TransportInformator