#pragma once

#include <systemc.h>

#include "eie_if.h"
#include "eie_central_control.h"

/*************************************************************
EIE_Accelerator.h is the modelled EIE accelerator unit. Since
it is just a model, this module does not actually implement
the sparse matrix multipy as specified by the EIE paper. 
Instead each weight is anyway transferred to the accelerators
whole, and the accelerator takes care of the computation as
a neural network normally would. Weights are stored in a 
local SRAM which is local to each accelerator. No weight
sharing occurs between accelerators. This is a model solely
for the fully-connected layers of a network.

POWER MODELLING:
	Power modelling is carried out with Yousef's power 
	estimates which are based both on the EIE paper and some
	approximations we made. Each local SRAM access costs 5 pJ
	and the computations are as follows:
		- 32-bit int add = 0.1 pJ
		- 32-bit float add = 0.9 pJ
		- 32-bit int multiply = 3.1 pJ
		- 32-bit float multiply = 3.7 pJ

*************************************************************/


class EIE_accelerator : public sc_module, public EIE_accel_if {
private:
    std::vector<std::vector<std::vector<double>>> weightSRAM;
    std::vector<double> input;
    std::vector<double> output;

    bool input_ready, output_ready;
    unsigned int current_layer;
public:
    sc_in_clk clk;
	
	int tally_sram_access, tally_float_add, tally_float_multiply;
	
    SC_HAS_PROCESS(EIE_accelerator);

    EIE_accelerator(sc_module_name name) : sc_module(name) {
        input_ready = false;
        output_ready = false;
		
		tally_sram_access = 0;
		tally_float_add = 0;
		tally_float_multiply = 0;
		

        SC_THREAD(eie_accelerator_proc);
    }

    void eie_accelerator_proc() {
        while (true) {
            wait(clk.posedge_event());

            if (input_ready) {
                input_ready = false;
                std::vector<std::vector<double>> &layerWeights = weightSRAM.at(current_layer);
                if (layerWeights.at(0).size() != input.size()) {
                    continue;
                }
                output.clear();
                for (int i = 0; i < layerWeights.size(); i++) {
                    double matVecProd = 0.0;
                    for (int j = 0; j < layerWeights.at(i).size(); j++) {
                        double mtmp = layerWeights.at(i).at(j);
                        double vtmp = input.at(j);
                        if (mtmp != 0 && vtmp != 0) {
                            matVecProd += layerWeights.at(i).at(j) * input.at(j);
                        } else {
                            // skip zeros
                        }
                    }
					tally_sram_access += (int) (2 * layerWeights.size() * layerWeights.at(i).size());
					tally_float_add += (int) (layerWeights.size() * layerWeights.at(i).size());
					tally_float_multiply += (int) (layerWeights.size() * layerWeights.at(i).size());
                    output.push_back(std::max(0.0, matVecProd));
                }
				
                output_ready = true;
            }
            
        }
    }

    bool PushWeights(std::vector<double> &weights, unsigned int layer) {
        std::vector<double> weightCopy(weights);
        while (weightSRAM.size() < layer + 1) {
            weightSRAM.push_back(std::vector<std::vector<double>>());
        }
        weightSRAM.at(layer).push_back(weightCopy);

        return true;
    }

    bool PushInputs(std::vector<double> &inputs, unsigned int layer) {
        output_ready = false;
        
        input.clear();
        for (int i = 0; i < inputs.size(); i++) {
            input.push_back(inputs.at(i));
        }
        current_layer = layer;
        
        input_ready = true;
        return true;
    }

    bool FetchResult(std::vector<double> &result) {
        result.clear();
        
        while (!output_ready) {
            wait(clk.posedge_event());
        }
        
        for (int i = 0; i < output.size(); i++) {
            result.push_back(output.at(i));
        }
        return true;
    }

    void PrintAcceleratorInfo(int accelerator_id) {
        cout << "Accelerator " << accelerator_id << endl;
        cout << weightSRAM.size() << " Layers:" << endl;
        for (int i = 0; i < weightSRAM.size(); i++) {
            cout << "Layer " << i << " - " << weightSRAM.at(i).size() << " rows, ";
            cout << weightSRAM.at(i).at(0).size() << " columns" << endl;
            cout << "First few weights: " << weightSRAM[i][0][0] << " " << weightSRAM[i][0][1] << " " << weightSRAM[i][0][2] << endl;
        }
        cout << endl;
    }
};
