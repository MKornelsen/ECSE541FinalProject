#pragma once

#include <systemc.h>

/*************************************************************
EIE_SW_Module.h is the CPU module of the EIE system. This 
module starts the initialization process with the control unit
by notifying it to start loading weights. After this is done, 
the module will load input images to the control unit for 
testing. The results of the test are loaded from the 
accelerator control unit and the correct label is loaded from
DRAM where it is stored with the input images. Labels are 
then compared and a tally is kept of the accuracy.

POWER MODELLING:
	Power modelling is carried out with Yousef's power 
	estimates which are based both on the EIE paper and some
	approximations we made. For the CPU, power is mostly 
	expended in the calculation phases. Tallies for bus power
	are kept in the bus and not the CPU. The power values for
	all operations are:
		- 32-bit int add = 0.1 pJ
		- 32-bit float add = 0.9 pJ
		- 32-bit int multiply = 3.1 pJ
		- 32-bit float multiply = 3.7 pJ

*************************************************************/

class EIE_SW_module : public sc_module {
private:
    unsigned int layerDefs[NUM_LAYERS + 1] = LAYER_SIZES;

public:
    sc_port<bus_master_if> bus;
	int tally_dram_access, tally_int_add, tally_int_multiply;
	
	
    SC_HAS_PROCESS(EIE_SW_module);

    EIE_SW_module(sc_module_name name) : sc_module(name) {
		
		tally_dram_access = 0;
		tally_int_add = 0;
		tally_int_multiply = 0;
		
        SC_THREAD(sw_proc);
    }

    void sw_proc() {
        cout << "EIE_SW running" << endl;
        unsigned int req_addr, req_op, req_len;
		
		//Add 640 pJ penalty for presumed reading of cpu instructions from DRAM. 
        tally_dram_access += 1;
		
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
            
            cout << "LAYER 0 DRAM_ADDR = " << dram_addr << endl;
            dram_addr += insize * outsize;
        }
        cout << "dram_addr = " << dram_addr << " after weight loading" << endl;
        // Start pushing the MNIST inputs
        // set cc status to EIE_CC_OP_WRITE_INPUT
        
		unsigned int goodPredictions = 0;

        for (int i = 0; i < TEST_IMAGES; i++) {
            req_addr = EIE_CC_BASE_ADDR;
            req_len = 10;
            req_op = OP_WRITE;
            
            ccstatus[EIE_CC_ADDR_OP] = EIE_CC_OP_WRITE_INPUT;
            ccstatus[EIE_CC_ADDR_DATA] = dram_addr;
            ccstatus[EIE_CC_ADDR_ROWLEN] = 28 * 28;

            bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
            bus->WaitForAcknowledge(BUS_MST_SW);
            for (unsigned int j = 0; j < req_len; j++) {
                bus->WriteData(ccstatus[j]);
            }

            req_addr = EIE_CC_BASE_ADDR + EIE_CC_ADDR_OUTREADY;
            req_len = 1;
            req_op = OP_READ;
            
            unsigned int done = 0;
            while (!done) {
                bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
                bus->WaitForAcknowledge(BUS_MST_SW);
                bus->ReadData(done);
            }

            req_addr = DRAM_BASE_ADDR + dram_addr + 28 * 28;
            req_len = 1;
            req_op = OP_READ;

            unsigned int correctLabel;
            bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
            bus->WaitForAcknowledge(BUS_MST_SW);
            bus->ReadData(correctLabel);

            req_addr = EIE_CC_BASE_ADDR + EIE_CC_ADDR_PREDICTED_LABEL;
            req_len = 1;
            req_op = OP_READ;

            unsigned int predLabel;
            bus->Request(BUS_MST_SW, req_addr, req_op, req_len);
            bus->WaitForAcknowledge(BUS_MST_SW);
            bus->ReadData(predLabel);

            cout << "Image " << i << endl;
            cout << "Correct Label: " << correctLabel << endl;
            cout << "Predicted Label: " << predLabel << endl << endl;;
            
            if (correctLabel == predLabel) {
                goodPredictions++;
				tally_int_add += 1;
            }

            dram_addr += 28 * 28 + 1;
			
			//tracking operations
			tally_int_add += 2;
			tally_int_multiply += 1;
        }
        
        cout << "Predicted " << goodPredictions << "/" << TEST_IMAGES << " (" << (double) goodPredictions / TEST_IMAGES << ")" << endl;
        //TODO: find time of execution for simulation, return to main. 
	//DON'T STOP EXECUTION .. This is static power. 
        sc_stop();
    }
};
