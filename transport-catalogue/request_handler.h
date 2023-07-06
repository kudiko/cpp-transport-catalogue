#pragma once

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "domain.h"

#include <iostream>
#include <sstream>
#include <string>

namespace TransportInformator
{

namespace ReqHandler
{
    class RequestHandler {
        public:
            // MapRenderer понадобится в следующей части итогового проекта
            RequestHandler(const Core::TransportCatalogue& db, Render::MapRenderer& renderer);

            // Возвращает информацию о маршруте (запрос Bus)
            std::optional<Core::BusInfo> GetBusStat(const std::string_view& bus_name) const;

            // Возвращает маршруты, проходящие через остановку
            const std::set<std::string_view> GetBusesByStop(const std::string_view& stop_name) const;

            // Этот метод будет нужен в следующей части итогового проекта
            const svg::Document& RenderMap();

        private:
            // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
            const Core::Stop* FindLastStop(std::string_view bus_name) const;

            const Core::TransportCatalogue& db_;
            Render::MapRenderer& renderer_;
    };


}


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
            bool is_roundtrip;
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