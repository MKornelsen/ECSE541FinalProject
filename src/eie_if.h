#pragma once

#include <systemc.h>

class EIE_accel_if : virtual public sc_interface {
public:
    virtual bool PushWeights(std::vector<double> &weights, unsigned int layer) = 0;
    virtual bool PushInputs(std::vector<double> &inputs, unsigned int layer) = 0;
    virtual bool FetchResult(std::vector<double> &result) = 0;
    virtual void PrintAcceleratorInfo(int accelerator_id) = 0;
};
