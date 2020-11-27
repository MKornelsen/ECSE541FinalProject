#pragma once

#include <systemc.h>

#include "eie_if.h"
#include "eie_central_control.h"

class EIE_accelerator : public sc_module, public EIE_accel_if {
private:
    std::vector<std::vector<std::vector<double>>> weightSRAM;
    std::vector<double> input;
    std::vector<double> output;

    bool input_ready, output_ready;
    unsigned int current_layer;
public:
    sc_in_clk clk;

    SC_HAS_PROCESS(EIE_accelerator);

    EIE_accelerator(sc_module_name name) : sc_module(name) {
        input_ready = false;
        output_ready = false;

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
        }
        cout << endl;
    }
};
