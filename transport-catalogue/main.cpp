#include <fstream>
#include <iostream>
#include <string_view>

#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {

        TransportInformator::Core::TransportCatalogue tc;
        TransportInformator::Input::JSONReader jsonreader{tc, std::cin};
        jsonreader.ReadMakeBaseJSON();
        TransportInformator::Serialize::Serializator serializer{tc, jsonreader.GetSerializationSettings()};
        TransportInformator::Router::TransportRouter router(tc, jsonreader.GetRouterSettings());
        serializer.SerializeToFile(jsonreader.GetRenderSettings(),
                                   jsonreader.GetRouterSettings(), router);

    } else if (mode == "process_requests"sv) {

        TransportInformator::Core::TransportCatalogue tc;
        TransportInformator::Input::JSONReader jsonreader{tc, std::cin};
        jsonreader.ReadProcessRequestsJSON();
        TransportInformator::Serialize::Serializator serializer{tc, jsonreader.GetSerializationSettings()};
        TransportInformator::Router::TransportRouter router = serializer.UnserializeFromFile();

        TransportInformator::Render::MapRenderer renderer(serializer.GetRenderSettings(), tc.GetAllNonEmptyStopsCoords());
        TransportInformator::ReqHandler::RequestHandler handler(tc, renderer, router);

        jsonreader.SendStatRequests(handler);

    } else {
        PrintUsage();
        return 1;
    }
}