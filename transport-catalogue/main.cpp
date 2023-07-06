
#include "transport_catalogue.h"
#include "json_reader.h"
#include "request_handler.h"
#include <fstream>
int main() {
    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
     * с ответами.
     * Вывести в stdout ответы в виде JSON
     */
    
    TransportInformator::Core::TransportCatalogue tc;
    TransportInformator::Input::JSONReader jsonreader{tc, std::cin};
    jsonreader.ReadJSON();
    TransportInformator::Render::MapRenderer renderer(jsonreader.GetRenderSettings(), tc.GetAllNonEmptyStopsCoords());
    TransportInformator::ReqHandler::RequestHandler handler(tc, renderer);

    jsonreader.SendStatRequests(handler);

   // const svg::Document& result = handler.RenderMap();
  //  result.Render(std::cout);
    
    /*
    json::Document right_answer = json::Load(std::cin);

    std::fstream f("output1.json");
    json::Document answer = json::Load(f);
    
    std::cout << bool(answer == right_answer) << std::endl;
    */
    
    return 0;
}