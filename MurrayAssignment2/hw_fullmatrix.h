#pragma once

#include <systemc.h>
#include "memmap.h"

class hw : public sc_module {
private:
    unsigned int bus_mst_id;
    unsigned int bus_addr, bus_op, bus_len;
    unsigned int listen_addr, listen_op, listen_len;
    unsigned int data;

    // {a_addr, b_addr, c_addr, x, ready}
    unsigned int state_buf[HW_DATA_SIZE];
    unsigned int a_buf[MAT_SIZE * MAT_SIZE], b_buf[MAT_SIZE * MAT_SIZE], c_buf[MAT_SIZE * MAT_SIZE];
    // unsigned int cur_row, cur_col;
    // unsigned int row_buf[MAT_SIZE], col_buf[MAT_SIZE];

    sc_event start_compute;

public:
    sc_port<bus_master_if> hw_bus_master;
    sc_port<bus_minion_if> hw_bus_minion;

    SC_HAS_PROCESS(hw);

    hw(sc_module_name name, unsigned int id) : sc_module(name), bus_mst_id(id) {
        SC_THREAD(hw_minion_proc);
        SC_THREAD(hw_master_proc);
    }

    void hw_minion_proc() {
        while (true) {
            hw_bus_minion->Listen(listen_addr, listen_op, listen_len);
            if (listen_addr >= HW_ADDR && listen_addr < HW_ADDR + HW_DATA_SIZE) {
                cout << "Listen: (" << listen_addr << ", " << listen_op << ", " << listen_len << ")" << endl;
                hw_bus_minion->Acknowledge();
                switch (listen_op) {
                case OP_SINGLE_READ:
                    hw_bus_minion->SendReadData(state_buf[listen_addr - HW_ADDR]);
                    cout << "hw - SendReadData" << endl;
                    if (listen_addr - HW_ADDR == HW_OUT_READY_POS) {
                        state_buf[HW_OUT_READY_POS] = 0;
                    }
                    break;
                case OP_SINGLE_WRITE:
                    hw_bus_minion->ReceiveWriteData(data);
                    state_buf[listen_addr - HW_ADDR] = data;
                    start_compute.notify();
                    break;
                case OP_BURST_READ:
                    for (unsigned int i = 0; i < listen_len; i++) {
                        hw_bus_minion->SendReadData(state_buf[listen_addr - HW_ADDR + i]);
                    }
                    break;
                case OP_BURST_WRITE:
                    for (unsigned int i = 0; i < listen_len; i++) {
                        hw_bus_minion->ReceiveWriteData(data);
                        state_buf[listen_addr - HW_ADDR + i] = data;
                        cout << "hw - burst write " << data << endl;
                    }
                    start_compute.notify();
                    break;
                }
            }
        }
    }

    void hw_master_proc() {
        while (true) {
            wait(start_compute);
            cout << "a_addr = " << state_buf[HW_ROW_POS] << ", b_addr = " << state_buf[HW_COL_POS] << endl;
            bool row_valid = false, col_valid = false;
            if (state_buf[HW_ROW_POS] < MEM_SIZE) {
                read_matrix(state_buf[HW_ROW_POS], a_buf);
                row_valid = true;
            }
            if (state_buf[HW_COL_POS] < MEM_SIZE) {
                read_matrix(state_buf[HW_COL_POS], b_buf);
                col_valid = true;
            }
            
            if (row_valid && col_valid) {
                // state_buf[HW_OUT_POS] = 0;
                // for (int i = 0; i < MAT_SIZE; i++) {
                //     state_buf[HW_OUT_POS] += row_buf[i] * col_buf[i];
                // }
                for (int i = 0; i < MAT_SIZE; i++) {
                    for (int j = 0; j < MAT_SIZE; j++) {
                        c_buf[i * MAT_SIZE + j] = 0;
                        for (int k = 0; k < MAT_SIZE; k++) {
                            c_buf[i * MAT_SIZE + j] += a_buf[i * MAT_SIZE + k] * b_buf[k * MAT_SIZE + j];
                        }
                    }
                }

                bus_addr = MEM_BASE_ADDR + state_buf[HW_OUT_POS];
                bus_op = OP_BURST_WRITE;
                bus_len = MAT_SIZE * MAT_SIZE;
                hw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
                hw_bus_master->WaitForAcknowledge(bus_mst_id);
                for (int i = 0; i < MAT_SIZE * MAT_SIZE; i++) {
                    hw_bus_master->WriteData(c_buf[i]);
                }

            }
            state_buf[HW_OUT_READY_POS] = 1;
        }
    }

    void read_matrix(unsigned int addr, unsigned int *store) {
        bus_addr = MEM_BASE_ADDR + addr;
        bus_op = OP_BURST_READ;
        bus_len = MAT_SIZE * MAT_SIZE;
        hw_bus_master->Request(bus_mst_id, bus_addr, bus_op, bus_len);
        hw_bus_master->WaitForAcknowledge(bus_mst_id);
        for (int i = 0; i < MAT_SIZE * MAT_SIZE; i++) {
            hw_bus_master->ReadData(data);
            store[i] = data;
        }
    }
};