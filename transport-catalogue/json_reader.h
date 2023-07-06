#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"


/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace TransportInformator
{

namespace Input
{
    enum class BaseRequestType
    {
        BUS,
        STOP
    };

    //Requests forming the database
    struct BaseRequest
    {
        BaseRequest(BaseRequestType new_type, const std::string& new_name);
        BaseRequestType type;
        std::string name;

        virtual ~BaseRequest() = default;

    };

    struct AddBusRequest : public BaseRequest
    {
        AddBusRequest(BaseRequestType new_type, const std::string& new_name, const std::vector<std::string>& new_stops, bool is_roundtrip);
        std::vector<std::string> stops;
        bool is_roundtrip;
    };

    struct AddStopRequest : public BaseRequest
    {
        AddStopRequest(BaseRequestType new_type, const std::string& new_name, 
        detail::Coordinates new_coords, const std::vector<std::pair<std::string, double>>& distances);
        detail::Coordinates coords;
        std::vector<std::pair<std::string, double>> distances_to_other_stops;
    };

    enum class StatRequestType
    {
        BUS,
        STOP,
        MAP,
    };

    class JSONReader;

    //Queries to already complete db
    struct StatRequest
    {
        StatRequest(size_t new_id, StatRequestType new_type);
        size_t id;
        StatRequestType type;

        virtual ~StatRequest() = default;
        virtual void Process(JSONReader& jreader, ReqHandler::RequestHandler& rh) = 0;
        
    };

    struct BusInfoRequest : public StatRequest
    {
        BusInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name);
        std::string name;
        void Process(JSONReader& jreader, ReqHandler::RequestHandler& rh) override;
    };

    struct StopInfoRequest : public StatRequest
    {
        StopInfoRequest(size_t new_id, StatRequestType new_type, std::string new_name);
        std::string name;
        void Process(JSONReader& jreader, ReqHandler::RequestHandler& rh) override;
    };
    
    struct MapRenderRequest : public StatRequest
    {
        MapRenderRequest(size_t new_id, StatRequestType new_type);
        void Process(JSONReader& jreader, ReqHandler::RequestHandler& rh) override;
    };

    struct DBCommands
    {
        struct BaseRequests
        {
            std::vector<AddBusRequest> add_bus;
            std::vector<AddStopRequest> add_stop;
        };
        BaseRequests base_requests;
        std::vector<StatRequest> stat_requests;
    };

    class JSONReader
    {
        public:
        JSONReader(Core::TransportCatalogue& tc, std::istream& in);
        void ReadJSON();
        void Print(std::ostream& out);
        Render::RenderSettings GetRenderSettings() const;
        void SendStatRequests(ReqHandler::RequestHandler& rh);

        private:
        Core::TransportCatalogue &tc_;
        std::istream& in_;

        friend class BusInfoRequest;
        friend class StopInfoRequest;
        friend class MapRenderRequest;

        void ProcessBaseRequests(const json::Array& arr);
        void ProcessAddStop(const json::Dict& add_stop_command);
        void ProcessAddBus(const json::Dict& add_bus_command);

        void SetDistancesToOtherStops(std::string_view start, const std::vector<std::pair<std::string, double>>& distances_to_other_stops);

        void SendBaseRequests();

        void AddStatRequests(const json::Array& arr);

        

        void ProcessRenderSettings(const json::Dict& render_settings);
        svg::Color ParseColorFromJSON(const json::Node& node) const;

        std::vector<std::unique_ptr<StatRequest>> stat_requests_;
        std::vector<json::Node> db_answers_;

        DBCommands commands_;

        Render::RenderSettings render_settings_;

        
        
    };
}

}