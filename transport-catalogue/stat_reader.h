#pragma once

#include "transport_catalogue.h"

namespace TransportInformator
{

namespace Output
{

    class StatReader
    {
    public:
        StatReader(const Core::TransportCatalogue &tc);

        void ReadInput();
        void ReadInput(std::istream &stream, std::ostream &out);

    private:
        void ProcessCommand(std::string_view command);
        void ProcessCommand(std::string_view command, std::ostream &out);
        void DeliverCommands();

        const Core::TransportCatalogue &tc_;
    };
    
} // namespace TransportInformator::Output

} // namespace TransportInformator