#pragma once 

#include <systemc.h>

#include "project_include.h"
#include "eie_if.h"

class EIE_central_control : public sc_module {
private:

    sc_event op_receive_event;
    sc_event network_execute_event;

    unsigned int status[EIE_CC_ADDR_SIZE];

    unsigned int numLayers;

    std::vector<double> inputBuffer;
    std::vector<double> outputBuffer;
public:
    sc_in_clk clk;

    sc_port<EIE_accel_if> accelerators[NUM_ACCELERATORS];
    sc_port<bus_minion_if> bus_minion;
    sc_port<bus_master_if> bus_master;

    SC_HAS_PROCESS(EIE_central_control);

    EIE_central_control(sc_module_name name) : sc_module(name) {
        numLayers = 0;
        for (int i = 0; i < EIE_CC_ADDR_SIZE; i++) {
            status[i] = 0;
        }
        
        SC_THREAD(eie_cc_minion);
        SC_THREAD(eie_cc_master);
        SC_THREAD(network_execute);
    }

    void eie_cc_minion() {
        unsigned int req_addr, req_op, req_len;
        while (true) {
            bus_minion->Listen(req_addr, req_op, req_len);
            if (req_addr >= EIE_CC_BASE_ADDR && req_addr < EIE_CC_BASE_ADDR + EIE_CC_ADDR_SIZE) {
                bus_minion->Acknowledge();

                unsigned int tmp_addr = req_addr - EIE_CC_BASE_ADDR;

                if (req_op == OP_READ) {
                    for (unsigned int i = 0; i < req_len; i++) {
                        bus_minion->SendReadData(status[tmp_addr + i]);
                    }
                } else if (req_op == OP_WRITE) {
                    for (unsigned int i = 0; i < req_len; i++) {
                        bus_minion->ReceiveWriteData(status[tmp_addr]);
                    }
                    
                    status[EIE_CC_ADDR_OP_COMPLETE] = 0;
                    if (tmp_addr == EIE_CC_ADDR_OP) {
                        op_receive_event.notify();
                    }
                }
            }
        }
    }

    void eie_cc_master() {
        unsigned int req_addr, req_op, req_len, data;

        while (true) {
            wait(op_receive_event);

            unsigned int data_addr = status[EIE_CC_ADDR_DATA];
            unsigned int layer = status[EIE_CC_ADDR_LAYER];
            unsigned int rowlen = status[EIE_CC_ADDR_ROWLEN];
            unsigned int rows = status[EIE_CC_ADDR_ROWS];

            switch (status[EIE_CC_ADDR_OP]) {
            case EIE_CC_OP_WRITE_WEIGHT:
                cout << "EIE_CC_OP_WRITE_WEIGHT" << endl;
                if (layer + 1 > numLayers) {
                    numLayers = layer + 1;
                }
                req_addr = data_addr + DRAM_BASE_ADDR;
                req_len = rows * rowlen;
                req_op = OP_READ;
                bus_master->Request(BUS_MST_HW, req_addr, req_op, req_len);
                bus_master->WaitForAcknowledge(BUS_MST_HW);
                for (unsigned int i = 0; i < rows; i++) {
                    std::vector<double> tmpWeights;
                    for (unsigned int j = 0; j < rowlen; j++) {
                        bus_master->ReadData(data);
                        double dval = (double) *(float *) &data;
                        tmpWeights.push_back(dval);
                    }
                    accelerators[i % NUM_ACCELERATORS]->PushWeights(tmpWeights, layer);
                }
                status[EIE_CC_ADDR_OP_COMPLETE] = 1;
                
                for (int i = 0; i < NUM_ACCELERATORS; i++) {
                    accelerators[i]->PrintAcceleratorInfo(i);
                }
                break;
            case EIE_CC_OP_READ_OUTPUT:
                req_addr = data_addr + DRAM_BASE_ADDR;
                req_len = (unsigned int) outputBuffer.size();
                req_op = OP_WRITE;
                bus_master->Request(BUS_MST_HW, req_addr, req_op, req_len);
                bus_master->WaitForAcknowledge(BUS_MST_HW);
                for (unsigned int i = 0; i < req_len; i++) {
                    float fd = (float) outputBuffer.at(i);
                    unsigned int ud = *(unsigned int *) &fd;
                    bus_master->WriteData(ud);
                }
                break;
            case EIE_CC_OP_WRITE_INPUT:
                status[EIE_CC_ADDR_OUTREADY] = 0;
                req_addr = data_addr + DRAM_BASE_ADDR;
                req_len = rowlen;
                req_op = OP_READ;
                bus_master->Request(BUS_MST_HW, req_addr, req_op, req_len);
                bus_master->WaitForAcknowledge(BUS_MST_HW);
                std::vector<double> tmpInput;
                for (unsigned int i = 0; i < req_len; i++) {
                    bus_master->ReadData(data);
                    double dval = (double) *(float *) &data;
                    tmpInput.push_back(dval);
                }
                network_execute_event.notify();
                break;
            }
        }
    }

    void network_execute() {
        while (true) {
            wait(network_execute_event);
            for (unsigned int i = 0; i < numLayers; i++) {
                for (int j = 0; j < NUM_ACCELERATORS; j++) {
                    accelerators[j]->PushInputs(inputBuffer, i);
                }
                std::vector<double> tmpOutput;
                std::vector<std::vector<double>> arrayForm;
                unsigned int outSize = 0;
                for (unsigned int i = 0; i < NUM_ACCELERATORS; i++) {
                    accelerators[i]->FetchResult(tmpOutput);
                    arrayForm.push_back(std::vector<double>(tmpOutput));
                    outSize += (unsigned int) tmpOutput.size();
                }
                inputBuffer.clear();
                for (unsigned int i = 0; i < outSize; i++) {
                    inputBuffer.push_back(arrayForm.at(i % NUM_ACCELERATORS).at(i / NUM_ACCELERATORS));
                }
            }
            outputBuffer.clear();
            for (unsigned int i = 0; i < inputBuffer.size(); i++) {
                outputBuffer.push_back(inputBuffer.at(i));
            }
            status[EIE_CC_ADDR_OUTREADY] = 1;
        }
    }
};