#pragma once 

#include <systemc.h>

#include "bus_if.h"
#include "memmap.h"

class memory : public sc_module {
private:
    unsigned int memBlock[MEM_SIZE];

public:
    sc_port<bus_minion_if> mem_bus;

    SC_HAS_PROCESS(memory);

    memory(sc_module_name name) : sc_module(name) {
        unsigned int a[36] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
        unsigned int b[36] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};

        // Modified a and b
        // unsigned int a[36] = {1,3,5,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
        // unsigned int b[36] = {2,4,6,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};

        for (int i = 0; i < 36; i++) {
            memBlock[A_ADDR + i] = a[i];
            memBlock[B_ADDR + i] = b[i];
            memBlock[C_ADDR + i] = 0;
        }

        SC_THREAD(mem_proc);
    }

    void mem_proc() {
        unsigned int listen_addr, listen_op, listen_len;
        unsigned int data;
        while (true) {
            mem_bus->Listen(listen_addr, listen_op, listen_len);
            // cout << "Listen: (" << listen_addr << ", " << listen_op << ", " << listen_len << ")" << endl;
            if (listen_addr >= MEM_BASE_ADDR && listen_addr < MEM_BASE_ADDR + MEM_SIZE) {
                cout << "Listen: (" << listen_addr << ", " << listen_op << ", " << listen_len << ")" << endl;
                mem_bus->Acknowledge();
                switch (listen_op) {
                case OP_SINGLE_READ:
                    data = memBlock[listen_addr - MEM_BASE_ADDR];
                    mem_bus->SendReadData(data);
                    break;
                case OP_SINGLE_WRITE:
                    mem_bus->ReceiveWriteData(data);
                    memBlock[listen_addr - MEM_BASE_ADDR] = data;
                    break;
                case OP_BURST_READ:
                    for (unsigned int i = 0; i < listen_len; i++) {
                        data = memBlock[listen_addr - MEM_BASE_ADDR + i];
                        mem_bus->SendReadData(data);
                    }
                    break;
                case OP_BURST_WRITE:
                    for (unsigned int i = 0; i < listen_len; i++) {
                        mem_bus->ReceiveWriteData(data);
                        memBlock[listen_addr - MEM_BASE_ADDR + i] = data;
                    }
                    break;
                }
            }
        }
    }
};
