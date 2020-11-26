#pragma once

#include <systemc.h>

#include "eie_if.h"
#include "eie_central_control.h"

class eie_accelerator : public sc_module {
private:
    std::vector<std::vector<std::vector<double>>> weightSRAM;
    std::vector<double> input;
    std::vector<double> output;

    bool output_ready;
public:
    sc_in_clk clk;

    SC_HAS_PROCESS(eie_accelerator);

    eie_accelerator(sc_module_name name) : sc_module(name) {
        output_ready = false;

        SC_THREAD(eie_accelerator_proc);
    }

    void eie_accelerator_proc() {
        while (true) {
            wait(clk.posedge_event());

        }
    }

    bool PushWeights(std::vector<double> weights, unsigned int layer) {
        std::vector<double> weightCopy(weights);
        weightSRAM.at(layer).push_back(weightCopy);
    }

    bool PushInputs(std::vector<double> inputs) {
        input.clear();
        for (int i = 0; i < inputs.size(); i++) {
            input.push_back(inputs.at(i));
        }
        return true;
    }

    bool ComputeResult(unsigned int layer, std::vector<double> &result) {
        result.clear();
        std::vector<std::vector<double>> &layerWeights = weightSRAM.at(layer);
        
        for (int i = 0; i < output.size(); i++) {
            result.push_back(output.at(i));
        }
        return true;
    }

};
