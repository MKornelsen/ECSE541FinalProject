#pragma once

#include <systemc.h>

class EIE_SW_module : public sc_module {
private:

public:
    sc_port<bus_master_if> bus;

    SC_HAS_PROCESS(EIE_SW_module);

    EIE_SW_module(sc_module_name name) : sc_module(name) {

    }
};