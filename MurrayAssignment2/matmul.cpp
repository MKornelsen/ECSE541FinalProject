#include <systemc.h>

#include "bus_unclocked.h"
#include "memory.h"
#include "sw_hw.h"
#include "hw.h"

class bus_tb : public sc_module {
public:
    bus_unclocked *bus_inst;
    sw *sw_inst;
    hw *hw_inst;
    memory *mem_inst;

    bus_tb(sc_module_name name) : sc_module(name) {
        bus_inst = new bus_unclocked("bus_inst");

        unsigned int tmp;
        bus_inst->attach_master(tmp);
        sw_inst = new sw("sw_inst", tmp);
        sw_inst->sw_bus_master(*bus_inst);
        cout << "sw mst_id = " << tmp << endl;

        bus_inst->attach_master(tmp);
        hw_inst = new hw("hw_inst", tmp);
        hw_inst->hw_bus_master(*bus_inst);
        hw_inst->hw_bus_minion(*bus_inst);
        cout << "hw mst_id = " << tmp << endl;

        mem_inst = new memory("mem_inst");
        mem_inst->mem_bus(*bus_inst);
    }
};

int sc_main(int argc, char *argv[]) {
    bus_tb tb("tb");
    sc_start();

    return 0;
}
