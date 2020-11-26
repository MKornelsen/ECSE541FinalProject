#pragma once 

#include <systemc.h>

#include "project_include.h"
#include "eie_if.h"

class EIE_central_control : public sc_module {
private:
    
public:
    sc_in_clk clk;

    sc_port<EIE_accel_if> accelerators[NUM_ACCELERATORS];
    sc_port<bus_minion_if> bus_minion;
    sc_port<bus_master_if> bus_master;

    SC_HAS_PROCESS(EIE_central_control);

    EIE_central_control(sc_module_name name) : sc_module(name) {

        SC_THREAD(eie_cc_proc);
    }

    void eie_cc_proc() {

    }
};
