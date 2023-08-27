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
#include "transport_router.h"
#include "serialization.h"

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
            RequestHandler(Core::TransportCatalogue& db, Render::MapRenderer& renderer, Router::TransportRouter& router);

            // Возвращает информацию о маршруте (запрос Bus)
            std::optional<Core::BusInfo> GetBusStat(const std::string_view& bus_name) const;

            // Возвращает маршруты, проходящие через остановку
            const std::set<std::string_view> GetBusesByStop(const std::string_view& stop_name) const;

            // Этот метод будет нужен в следующей части итогового проекта
            const svg::Document& RenderMap();

            std::optional<Router::Route> BuildRoute(std::string_view from, std::string_view to) const;


        private:
            // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"

            Core::TransportCatalogue& db_;
            Render::MapRenderer& renderer_;
            Router::TransportRouter& router_;
    };


} // namespace TransportInformator::ReqHandler

} // namespace TransportInformator