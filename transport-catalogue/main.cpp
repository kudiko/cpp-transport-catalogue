#include "tests.h"

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main()
{
    {
        TransportInformator::Tests::Tester tests;
    }
    
    TransportInformator::Core::TransportCatalogue tc;
    TransportInformator::Input::InputReader ir(tc);
    TransportInformator::Output::StatReader sr(tc);

    ir.ReadInput();
    sr.ReadInput();
    
    return 0;
}