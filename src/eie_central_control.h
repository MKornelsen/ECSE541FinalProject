#pragma once 

#include <systemc.h>

#include "project_include.h"
#include "eie_if.h"

/*************************************************************
EIE_Central_Control.h is the accelerator control unit. This
module connects to all the accelerators over local busses and
transfer weights and inputs/outputs. This module has two bus
connections: the on-chip bus connecting to the CPU, and the
bus to the accelerators. The former is already modelled in 
bus.h, and the latter must be modelled here. 

POWER MODELLING:
	Power modelling is carried out with Yousef's power 
	estimates which are based both on the EIE paper and some
	approximations we made. Each bus transfer on the 
	accelerator-facing bus costs 5.5 pJ as per the model. We
	also take into account the power due to register writes
	when taking the output from the accelerators. 
	


*************************************************************/

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
	
	unsigned int tally_transfers_acc_bus, tally_output_read;
	
    SC_HAS_PROCESS(EIE_central_control);

    EIE_central_control(sc_module_name name) : sc_module(name) {
        numLayers = 0;
        for (int i = 0; i < EIE_CC_ADDR_SIZE; i++) {
            status[i] = 0;
        }
		
		tally_output_read = 0;
		tally_transfers_acc_bus = 0;
        
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
                        bus_minion->ReceiveWriteData(status[tmp_addr + i]);
                    }
                    // cout << "write " << req_addr << endl;
                    status[EIE_CC_ADDR_OP_COMPLETE] = 0;
                    if (tmp_addr == EIE_CC_ADDR_OP) {
                        // cout << "op_receive_event notify" << endl;
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

            // cout << "op_receive_event caught" << endl;

            switch (status[EIE_CC_ADDR_OP]) {
            case EIE_CC_OP_WRITE_WEIGHT:
                cout << "EIE_CC_OP_WRITE_WEIGHT" << endl;
                if (layer + 1 > numLayers) {
                    numLayers = layer + 1;
                }
                req_addr = data_addr + DRAM_BASE_ADDR;
                cout << "req_addr = " << req_addr << endl;
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
                    // cout << "pushing row " << i << " layer " << layer << endl;
                    accelerators[i % NUM_ACCELERATORS]->PushWeights(tmpWeights, layer);
                }
                status[EIE_CC_ADDR_OP_COMPLETE] = 1;
                
                // for (int i = 0; i < NUM_ACCELERATORS; i++) {
                //     accelerators[i]->PrintAcceleratorInfo(i);
                // }
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
                // cout << "EIE_CC_OP_WRITE_INPUT" << endl;
                // for (int i = 0; i < NUM_ACCELERATORS; i++) {
                //     accelerators[i]->PrintAcceleratorInfo(i);
                // }
                status[EIE_CC_ADDR_OUTREADY] = 0;
                req_addr = data_addr + DRAM_BASE_ADDR;
                req_len = rowlen;
                req_op = OP_READ;
                bus_master->Request(BUS_MST_HW, req_addr, req_op, req_len);
                bus_master->WaitForAcknowledge(BUS_MST_HW);
                inputBuffer.clear();
                for (unsigned int i = 0; i < req_len; i++) {
                    bus_master->ReadData(data);
                    double dval = (double) *(float *) &data;
                    inputBuffer.push_back(dval);
                }
                network_execute_event.notify();
                break;
            }
        }
    }

    void network_execute() {
        while (true) {
            wait(network_execute_event);
            // cout << "network_execute_event received" << endl;
            // cout << "inputBuffer size = " << inputBuffer.size() << endl;
            // cout << "INPUT:" << endl;
            // for (unsigned int i = 0; i < inputBuffer.size(); i++) {
            //     cout << inputBuffer.at(i) << " ";
            // }
            // cout << endl;
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
			//TODO: Murray, you need to fix these tallies. I don't know how to find the size of these buffers.. 
			//For every element that is pushed to the accelerators, we need to add it to the tally. Since inputs
			//are pushed then outputs are pulled we need the size of input and the size of output for the transfer tally
			// and for the output read tally we need just the number of outputs. 
			//Keep track of the accelerator accesses
			tally_transfers_acc_bus += numLayers * (2 * NUM_ACCELERATORS); //tally of the presumed local accesses
			tally_output_read += numLayers * NUM_ACCELERATORS; //tally of the storage of outputs from the accelerators
			
            outputBuffer.clear();
            // cout << "OUTPUT:" << endl;
            for (unsigned int i = 0; i < inputBuffer.size(); i++) {
                outputBuffer.push_back(inputBuffer.at(i));
                // cout << outputBuffer.at(i) << " ";
            }
            // cout << endl;
            
            unsigned int maxidx = 0;
            for (unsigned int i = 1; i < outputBuffer.size(); i++) {
                if (outputBuffer[i] > outputBuffer[maxidx]) {
                    maxidx = i;
                }
            }
            status[EIE_CC_ADDR_PREDICTED_LABEL] = maxidx;
            status[EIE_CC_ADDR_OUTREADY] = 1;
        }
    }
};
