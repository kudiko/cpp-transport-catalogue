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

        void ReadInput();
        void ReadInput(std::istream &stream);

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
        void DeliverCommands();

        DBUpdateQueue queue_;
        Core::TransportCatalogue &tc_;
    };

} // namespace TransportInformator::Input

} // namespace TransportInformator