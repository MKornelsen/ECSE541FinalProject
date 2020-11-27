#include <SystemC.h>
#include <project_include.h>
#include "bus.cpp"
#include "eie_accelerator.h"

class accelerator_tb : public sc_module {
private:
    std::vector<std::vector<double>> weights;

public:
    // sc_signal<bool> clk;
    // sc_clock clk;
    sc_in_clk clk;
    EIE_accelerator *accelerator;

    sc_port<EIE_accel_if> acc_port;

    SC_HAS_PROCESS(accelerator_tb);

    accelerator_tb(sc_module_name name) : sc_module(name) {
        // cout << "Hello!\n" << endl;
        

        accelerator = new EIE_accelerator("ACC");
        accelerator->clk(clk);

        acc_port(*accelerator);

        double winit[5][5] = {{1, 2, 3, 4, 5}, 
                              {6, 7, 8, 9, 10},
                              {11, 12, 13, 14, 15},
                              {16, 17, 18, 19, 20},
                              {21, 22, 23, 24, 25}};

        // cout << "weights" << endl;
        for (int i = 0; i < 5; i++) {
            weights.push_back(std::vector<double>());
            for (int j = 0; j < 5; j++) {
                weights.at(i).push_back(winit[i][j]);
            }
        }
        // cout << "weights copied" << endl;

        SC_THREAD(test_proc);
    }

    void test_proc() {

        cout << "pushing weights" << endl;
        for (int i = 0; i < weights.size(); i++) {
            acc_port->PushWeights(weights.at(i), 0);
        }
        
        std::vector<double> testinput({5, 4, 3, 2, 1});

        cout << "correct product:" << endl;
        for (int i = 0; i < weights.size(); i++) {
            double cor = 0;
            for (int j = 0; j < weights.at(i).size(); j++) {
                cor += weights.at(i).at(j) * testinput.at(j);
            }
            cout << cor << " ";
        }
        cout << endl;
        
        cout << "pushing inputs" << endl;
        acc_port->PushInputs(testinput, 0);

        std::vector<double> result;
        acc_port->FetchResult(result);

        for (int i = 0; i < result.size(); i++) {
            cout << result[i] << " ";
        }
        cout << endl;

        sc_stop();
    }
};

int sc_main(int argc, char *argv[]) {
    sc_clock clk("CLOCK", 5, SC_NS);
    accelerator_tb tb("TOP");
    tb.clk(clk);

    sc_start();

    return 0;
}