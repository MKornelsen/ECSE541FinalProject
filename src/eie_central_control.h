#pragma once 

#include <systemc.h>

#include "eie_if.h"

class eie_central_control : public sc_module {
private:
    eie_accel_if accelerators[NUM_ACCELERATORS];

public:

    eie_central_control(sc_module_name name) : sc_module(name) {
        
    }    
};
   
};

};
