#pragma once

#include "transport_catalogue.h"

#include <iostream>

namespace TransportInformator
{

namespace Output
{

    class StatReader
    {
    public:
        StatReader(const Core::TransportCatalogue &tc);

        void ReadInput(std::istream &stream = std::cin, std::ostream &out = std::cout);

    private:
        void ProcessCommand(std::string_view command, std::ostream &out = std::cout);
        void PrintBusResult(std::stringstream& ss, std::ostream &out = std::cout);
        void PrintStopResult(std::stringstream& ss, std::ostream &out = std::cout);
        void DeliverCommands();

        const Core::TransportCatalogue &tc_;
    };
    
} // namespace TransportInformator::Output

} // namespace TransportInformator