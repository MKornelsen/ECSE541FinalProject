#include <systemc.h>

#include "bus_clocked.h"
#include "memory.h"
#include "sw_full_hw.h"
#include "hw_fullmatrix.h"

class bus_tb : public sc_module {
public:
    bus_clocked *bus_inst;
    sw *sw_inst;
    memory *mem_inst;
    hw *hw_inst;

    sc_signal<sc_logic> clk;

    SC_HAS_PROCESS(bus_tb);

    bus_tb(sc_module_name name) : sc_module(name) {
        bus_inst = new bus_clocked("bus_inst");
        bus_inst->clk(clk);

        unsigned int tmp;
        bus_inst->attach_master(tmp);
        sw_inst = new sw("sw_inst", tmp);
        sw_inst->sw_bus_master(*bus_inst);

        bus_inst->attach_master(tmp);
        hw_inst = new hw("hw_inst", tmp);
        hw_inst->hw_bus_master(*bus_inst);
        hw_inst->hw_bus_minion(*bus_inst);

        mem_inst = new memory("mem_inst");
        mem_inst->mem_bus(*bus_inst);

        SC_THREAD(clk_proc);
    }

    void clk_proc() {
        while (true) {
            clk.write(SC_LOGIC_0);
            wait(3333, SC_PS);
            clk.write(SC_LOGIC_1);
            wait(3333, SC_PS);
        }
    }
};

int sc_main(int argc, char *argv[]) {
    bus_tb tb("tb");
    sc_start();

    return 0;
}
