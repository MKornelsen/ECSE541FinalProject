#pragma once

#include <systemc.h>

class EIE_SW_module : public sc_module {
private:
    unsigned int layerDefs[NUM_LAYERS + 1] = LAYER_SIZES;

public:
    sc_port<bus_master_if> bus;

    SC_HAS_PROCESS(EIE_SW_module);

    EIE_SW_module(sc_module_name name) : sc_module(name) {

        SC_THREAD(sw_proc);
    }

    void sw_proc() {
        cout << "EIE_SW running" << endl;
        unsigned int req_addr, req_op, req_len;

        // load weights to accelerators
        unsigned int ccstatus[10];

        unsigned int dram_addr = 0;
        
        for (int i = 0; i < NUM_LAYERS; i++) {
            unsigned int insize = layerDefs[i];
            unsigned int outsize = layerDefs[i + 1];

            req_addr = EIE_CC_BASE_ADDR;
            req_len = 10;
            req_op = OP_WRITE;

            ccstatus[EIE_CC_ADDR_OP] = EIE_CC_OP_WRITE_WEIGHT;
            ccstatus[EIE_CC_ADDR_DATA] = dram_addr;
            ccstatus[EIE_CC_ADDR_LAYER] = i;
            ccstatus[EIE_CC_ADDR_ROWLEN] = insize;
            ccstatus[EIE_CC_ADDR_ROWS] = outsize;

            bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
            bus->WaitForAcknowledge(BUS_MST_SW);
            for (unsigned int j = 0; j < req_len; j++) {
                bus->WriteData(ccstatus[j]);
            }

            req_addr = EIE_CC_BASE_ADDR + EIE_CC_ADDR_OP_COMPLETE;
            req_len = 1;
            req_op = OP_READ;

            unsigned int done = 0;
            while (!done) {
                bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
                bus->WaitForAcknowledge(BUS_MST_SW);
                bus->ReadData(done);
            }
        }

        sc_stop();
    }
};