#pragma once

#include <systemc.h>

#include "bus_if.h"
#include "memmap.h"

class sw : public sc_module {
private:
    unsigned int bus_mst_id, bus_addr, bus_op, bus_len;
    
public: 
    sc_port<bus_master_if> sw_bus_master;

    SC_HAS_PROCESS(sw);

    sw(sc_module_name name, unsigned int id) : sc_module(name), bus_mst_id(id) {
        SC_THREAD(sw_proc);
    }

    void sw_proc() {
        unsigned int c, tmpA, tmpB;
        for (int i = 0; i < MAT_SIZE; i++) {
            for (int j = 0; j < MAT_SIZE; j++) {
                c = 0;
                for (int k = 0; k < MAT_SIZE; k++) {
                    cout << "(" << i << ", " << j << ", " << k << ")" << endl;
                    bus_addr = MEM_BASE_ADDR + A_ADDR + i * MAT_SIZE + k;
                    bus_op = OP_SINGLE_READ;
                    bus_len = 1;
                    sw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
                    sw_bus_master->WaitForAcknowledge(bus_mst_id);
                    sw_bus_master->ReadData(tmpA);

                    cout << "tmpA = " << tmpA << endl;

                    bus_addr = MEM_BASE_ADDR + B_ADDR + k * MAT_SIZE + j;
                    bus_op = OP_SINGLE_READ;
                    bus_len = 1;
                    sw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
                    sw_bus_master->WaitForAcknowledge(bus_mst_id);
                    sw_bus_master->ReadData(tmpB);

                    cout << "tmpB = " << tmpB << endl;

                    c += tmpA * tmpB;
                }
                cout << "Result[" << i << ", " << j << "] = " << c << endl;
                bus_addr = MEM_BASE_ADDR + C_ADDR + i * MAT_SIZE + j;
                bus_op = OP_SINGLE_WRITE;
                bus_len = 1;
                sw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
                sw_bus_master->WaitForAcknowledge(bus_mst_id);
                sw_bus_master->WriteData(c);
            }
        }

        cout << "Computation done - " << sc_time_stamp() << endl;
        
        bus_addr = MEM_BASE_ADDR + C_ADDR;
        bus_op = OP_BURST_READ;
        bus_len = MAT_SIZE * MAT_SIZE;
        sw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
        sw_bus_master->WaitForAcknowledge(bus_mst_id);
        
        unsigned int matProd[MAT_SIZE][MAT_SIZE];
        for (int i = 0; i < MAT_SIZE; i++) {
            for (int j = 0; j < MAT_SIZE; j++) {
                unsigned int x;
                sw_bus_master->ReadData(x);
                matProd[i][j] = x;
            }
        }

        for (int i = 0; i < MAT_SIZE; i++) {
            for (int j = 0; j < MAT_SIZE; j++) {
                cout << matProd[i][j] << " ";
            }
            cout << endl;
        }
        
        sc_stop();
    }
};
