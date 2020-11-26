#pragma once

#include <systemc.h>

class eie_accel_if : virtual public sc_interface {
public:
    virtual bool PushWeights(std::vector<double> weights, unsigned int layer) = 0;
    virtual bool PushInputs(std::vector<double> inputs) = 0;
    virtual bool ComputeResult(unsigned int layer, std::vector<double> &results) = 0;
};
