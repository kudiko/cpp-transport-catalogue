#include "tests.h"
#include "test_framework.h"
#include "input_reader.h"
#include "stat_reader.h"
#include <iomanip>



namespace TransportInformator
{

namespace Tests
{

using namespace Core;
using namespace Input;
using namespace Output;
using namespace detail;

Tester::Tester()
{
    RUN_TEST(tr, TestTransportCatalogue);
    RUN_TEST(tr, TestInputReader);
    RUN_TEST(tr, TestStatReader);
}

void Tester::TestTransportCatalogue()
{

    {
        TransportCatalogue ts;
        ts.AddStop("Flower", {2.0, 3.0});
        auto find_result = ts.FindStop("Flower");
        Stop expected {"Flower", {2.0, 3.0}};

        ASSERT_EQUAL(find_result->name, expected.name);
        ASSERT(find_result->coords == expected.coords);
    }

    {
        TransportCatalogue ts;
        ts.AddStop("Flower", {2.0, 3.0});
        ts.AddStop("Honey", {4.0, 5.0});
        ts.AddStop("Tree", {6.0, 7.0});
        
        ts.AddBus("001", {"Flower", "Honey", "Tree"});

        auto find_result = ts.FindBus("001");

        Bus expected {"001", {ts.FindStop("Flower"), ts.FindStop("Honey"), ts.FindStop("Tree")}};

        ASSERT_EQUAL(find_result->name, expected.name);
        ASSERT_EQUAL(find_result->stops, expected.stops);
    }

    {
        TransportCatalogue ts;
        ts.AddStop("Flower", {2.0, 2.0});
        ts.AddStop("Honey", {2.0, 3.0});

        ts.SetDistanceBetweenStops(ts.FindStop("Flower"), ts.FindStop("Honey"), 2.0);
        ts.SetDistanceBetweenStops(ts.FindStop("Honey"), ts.FindStop("Flower"), 4.0);

        ASSERT_EQUAL(ts.GetDistanceBetweenStops(ts.FindStop("Flower"), ts.FindStop("Honey")), 2.0);
        ASSERT_EQUAL(ts.GetDistanceBetweenStops(ts.FindStop("Honey"), ts.FindStop("Flower")), 4.0);

    }

    {
        TransportCatalogue ts;
        ts.AddStop("Flower", {2.0, 2.0});
        ts.AddStop("Honey", {2.0, 2.0});
        ts.AddStop("Tree", {2.0, 3.0});

        ts.SetDistanceBetweenStops(ts.FindStop("Flower"), ts.FindStop("Honey"), 2.0);
        ts.SetDistanceBetweenStops(ts.FindStop("Honey"), ts.FindStop("Tree"), 4.0);

        
        ts.AddBus("001", {"Flower", "Honey", "Tree"});

        ASSERT(ts.GetBusInfo("001"));
        
        BusInfo info_result = ts.GetBusInfo("001").value();

        double expected_real_distance = 2.0 + 4.0;
        BusInfo expected{"001", 3, 3, expected_real_distance, expected_real_distance / ComputeDistance({2.0, 2.0}, {2.0, 3.0}) };
        ASSERT_EQUAL(info_result.name, expected.name);
        ASSERT_EQUAL(info_result.stops, expected.stops);

        ASSERT_EQUAL(info_result.unique_stops, expected.unique_stops);
        ASSERT_EQUAL(info_result.route_length, expected.route_length);
        ASSERT_EQUAL(info_result.curvature, info_result.curvature);
    }
}

void Tester::TestInputReader()
{
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        
        std::stringstream input;

        input << "2\n";
        input << "Stop Tree: 55.0, 60.0, 100.0m to Flower\n";
        input << "Stop Flower: 55.0, 60.0, 200.0m to Tree";

        ir.ReadInput(input);

        Stop* stop_ptr1 = ts.FindStop("Tree");
        Coordinates expected_coordinates1{55.0, 60.0};
        
        ASSERT(stop_ptr1->coords == expected_coordinates1);
        ASSERT_EQUAL(stop_ptr1->name, "Tree");

        Stop* stop_ptr2 = ts.FindStop("Flower");

        Coordinates expected_coordinates2{55.0, 60.0};
        
        ASSERT(stop_ptr2->coords == expected_coordinates2);
        ASSERT_EQUAL(stop_ptr2->name, "Flower");

        ASSERT_EQUAL(ts.GetDistanceBetweenStops(stop_ptr1, stop_ptr2), 100.0);
        ASSERT_EQUAL(ts.GetDistanceBetweenStops(stop_ptr2, stop_ptr1), 200.0);

    }

    {
        TransportCatalogue ts;
        InputReader ir(ts);
        
        std::stringstream input;

        input << "2\n";
        input << "Stop Tree  Tree: 55.0, 60.0, 100.0m to Flower\n";
        input << "Stop Flower: 55.0, 60.0, 200.0m to Tree  Tree";

        ir.ReadInput(input);

        Stop* stop_ptr1 = ts.FindStop("Tree  Tree");
        ASSERT(stop_ptr1);
        Coordinates expected_coordinates1{55.0, 60.0};
        
        ASSERT(stop_ptr1->coords == expected_coordinates1);
        ASSERT_EQUAL(stop_ptr1->name, "Tree  Tree");

        Stop* stop_ptr2 = ts.FindStop("Flower");
        ASSERT(stop_ptr2);

        Coordinates expected_coordinates2{55.0, 60.0};
        
        ASSERT(stop_ptr2->coords == expected_coordinates2);
        ASSERT_EQUAL(stop_ptr2->name, "Flower");

        ASSERT_EQUAL(ts.GetDistanceBetweenStops(stop_ptr1, stop_ptr2), 100.0);
        ASSERT_EQUAL(ts.GetDistanceBetweenStops(stop_ptr2, stop_ptr1), 200.0);

    }
   
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        
        std::stringstream input;

        input << "1\n";
        input << "Stop Beautiful Tree: 55.0, 60";
        
        ir.ReadInput(input);

        Stop* stop_ptr = ts.FindStop("Beautiful Tree");
        Coordinates expected_coordinates{55.0, 60.0};
        
        ASSERT(stop_ptr->coords == expected_coordinates);
        ASSERT_EQUAL(stop_ptr->name, "Beautiful Tree");
    }

    {
        TransportCatalogue ts;
        InputReader ir(ts);
        
        std::stringstream input;

        input << "3\n";
        input << "Stop Tree: 55.0, 60\n";
        input << "Stop Flower: 55.0, 60\n";

        input << "Bus my bus: Flower > Tree";
        ir.ReadInput(input);

        Bus* bus_ptr = ts.FindBus("my bus");
        
        ASSERT(bus_ptr->name == "my bus");

        std::vector<const Stop*> expected{ts.FindStop("Flower"), ts.FindStop("Tree")};
        ASSERT_EQUAL(bus_ptr->stops, expected);
    }
    
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;
        input << "5\n";
        input << "Stop Flower: 55.0, 55.0\n";
        input << "Stop Tree: 50.0, 60.0\n";
        input << "Stop Honey: 60.0, 50.0\n";
        
        input << "Bus 001: Flower > Tree > Honey\n";
        input << "Bus 002: Flower - Tree - Honey\n";

        ir.ReadInput(input);

        std::vector<const Stop*> expected1{ts.FindStop("Flower"), ts.FindStop("Tree"), ts.FindStop("Honey")};
        std::vector<const Stop*> expected2{ts.FindStop("Flower"), ts.FindStop("Tree"), ts.FindStop("Honey"), 
        ts.FindStop("Tree"), ts.FindStop("Flower")};

        Bus* bus_ptr = ts.FindBus("001");
        ASSERT_EQUAL(bus_ptr->name, "001");
        ASSERT_EQUAL(bus_ptr->stops, expected1);

        bus_ptr = ts.FindBus("002");
        ASSERT_EQUAL(bus_ptr->name, "002");
        ASSERT_EQUAL(bus_ptr->stops, expected2);

    }
    //different order of buses and stops
    
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;
        input << "5\n";
        input << "Bus 001: Flower > Tree > Honey\n";
        input << "Stop Flower: 55.0, 55.0\n";
        input << "Stop Tree: 50.0, 60.0\n";
        input << "Bus 002: Flower - Tree - Honey\n";
        input << "Stop Honey: 60.0, 50.0\n";

        ir.ReadInput(input);

        std::vector<const Stop*> expected1{ts.FindStop("Flower"), ts.FindStop("Tree"), ts.FindStop("Honey")};
        std::vector<const Stop*> expected2{ts.FindStop("Flower"), ts.FindStop("Tree"), ts.FindStop("Honey"), 
        ts.FindStop("Tree"), ts.FindStop("Flower")};

        Bus* bus_ptr = ts.FindBus("001");
        ASSERT_EQUAL(bus_ptr->name, "001");
        ASSERT_EQUAL(bus_ptr->stops, expected1);

        bus_ptr = ts.FindBus("002");
        ASSERT_EQUAL(bus_ptr->name, "002");
        ASSERT_EQUAL(bus_ptr->stops, expected2);
    }
    
    
}

void Tester::TestStatReader()
{
    /*
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;
        input << "5\n";
        input << "Stop Flower: 55.0, 55.0\n";
        input << "Stop Tree: 50.0, 60.0\n";
        input << "Stop Honey: 60.0, 50.0\n";
        input << "Bus 001: Flower > Tree > Honey\n";
        input << "Bus 002: Flower - Tree - Honey\n";
        input << "3\n";
        input << "Bus 000\n";
        input << "Bus 001\n";
        input << "Bus 002";

        ir.ReadInput(input);

        StatReader sr(ts);

        std::stringstream out;

        sr.ReadInput(input, out);

        double expected_distance = ComputeDistance({55.0, 55.0}, {50.0, 60.0}) + ComputeDistance({60.0, 50.0}, {50.0, 60.0});
        std::stringstream expected;
        expected << "Bus 000: not found\nBus 001: 3 stops on route, 3 unique stops, " << std::setprecision(6) <<
        expected_distance << " route length\n" <<
        "Bus 002: 5 stops on route, 3 unique stops, " << 2 * expected_distance << " route length\n";

        ASSERT_EQUAL(expected.str(), out.str());

    }
    
    {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;

        input << "10\n";
        input << "Stop Tolstopaltsevo: 55.611087, 37.208290\n";
        input << "Stop Marushkino: 55.595884, 37.209755\n";
        input << "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n";
        input << "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n";
        input << "Stop Rasskazovka: 55.632761, 37.333324\n";
        input << "Stop Biryulyovo Zapadnoye: 55.574371, 37.651700\n";
        input << "Stop Biryusinka: 55.581065, 37.648390\n";
        input << "Stop Universam: 55.587655, 37.645687\n";
        input << "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n";
        input <<"Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164\n";
        input << "3\n";
        input <<"Bus 256\n";
        input <<"Bus 750\n";
        input <<"Bus 751";

        ir.ReadInput(input);

        StatReader sr(ts);

        std::stringstream out;
        sr.ReadInput(input, out);

        std::stringstream expected;
        expected << "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length\n" <<
        "Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length\n" <<
        "Bus 751: not found\n";

        ASSERT_EQUAL(out.str(), expected.str());
    }
    
     {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;
        input << "6\n";
        input << "Stop Flower: 55.0, 55.0\n";
        input << "Stop Tree: 50.0, 60.0\n";
        input << "Stop Honey: 60.0, 50.0\n";
        input << "Stop Test: 0, 0\n";
        input << "Bus 001: Flower > Tree > Honey\n";
        input << "Bus 002: Flower - Tree - Honey\n";
        input << "3\n";
        input << "Stop unknown stop\n";
        input << "Stop Flower\n";
        input << "Stop Test\n";

        ir.ReadInput(input);

        StatReader sr(ts);

        std::stringstream out;
        sr.ReadInput(input, out);

        std::stringstream expected;
        expected << "Stop unknown stop: not found\n" <<
        "Stop Flower: buses 001 002\n" <<
        "Stop Test: no buses\n";

        ASSERT_EQUAL(out.str(), expected.str());

    }*/



    {
        TransportCatalogue ts;
        InputReader ir(ts);
        std::stringstream input;
        input << "13\n";
        input << "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino\n";
        input << "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n";
        input << "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n";
        input << "Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka\n";
        input << "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino\n";
        input << "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n";
        input << "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam\n";
        input << "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n";
        input << "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n";
        input << "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n";
        input << "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n";
        input << "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n";
        input << "Stop Prazhskaya: 55.611678, 37.603831\n";
        input << "6\n";
        input << "Bus 256\n";
        input << "Bus 750\n";
        input << "Bus 751\n";
        input << "Stop Samara\n";
        input << "Stop Prazhskaya\n";
        input << "Stop Biryulyovo Zapadnoye";

        ir.ReadInput(input);

        StatReader sr(ts);

        std::stringstream out;
        sr.ReadInput(input, out);

        std::stringstream expected;

        expected << "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n";
        expected << "Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature\n";
        expected << "Bus 751: not found\n";
        expected << "Stop Samara: not found\n";
        expected << "Stop Prazhskaya: no buses\n";
        expected << "Stop Biryulyovo Zapadnoye: buses 256 828\n";

        ASSERT_EQUAL(out.str(), expected.str());

    }
}

} // namespace TransportInformator::Tests 

} // namespace TransportInformator