#pragma once

#include "transport_catalogue.h"
#include "test_framework.h"

namespace TransportInformator
{

namespace Tests
{

class Tester
{
    public:
    Tester();

    private:
    static void TestTransportCatalogue();
    static void TestInputReader();
    static void TestStatReader();

    TestRunner tr;
};

} // namespace TransportInformator::Tests

} // namespace TransportInformator