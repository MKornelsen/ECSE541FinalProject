#include <systemc.h>
#include <project_include.h>
#include "bus.h"
#include "cross_bus_module.cpp"
#include "memory.cpp"
#include "DRAM.cpp"
#include "eie_accelerator.h"
#include "eie_sw_module.h"

#define help_usage 1
#define help_file 2

/**
===========================================================================================
TODO:Change all comments and file names to match project
===========================================================================================
**/

#define clock_period_int 10
#define clock_period_ex  20

//Top module
class project_top : public sc_module {
	
	private:
		
	public:
		//Signals and ports
		sc_in_clk int_clk;
		sc_in_clk ext_clk;
		// sc_signal<bool> req_1;
		// sc_signal<bool> done, show_output;
		// sc_signal<int> sw_cycles, hw_cycles;
		
		//Object references
		bus_clocked * bus;
		EIE_SW_module * eie_sw;
		Cross_Bus * cross_bus;
		DRAM      * dram;
		EIE_central_control * eie_cc;
		EIE_accelerator * eie_accels[NUM_ACCELERATORS];
		
		SC_HAS_PROCESS(project_top);
		
		//Top module constructor
		project_top(sc_module_name name, bool verbose) : sc_module(name) {
			init_print();
			
			//Instantiate the objects and link them to the various ports and signals
			bus = new bus_clocked("MY_BUS");
			bus->clk(int_clk);
			unsigned int idtmp;
			bus->attach_master(idtmp);
			bus->attach_master(idtmp);

            eie_sw = new EIE_SW_module("EIE_SW");
            eie_sw -> bus(*bus);
			
			dram = new DRAM("MY_DRAM");
			
			cross_bus = new Cross_Bus("MY_INTERNAL_EXTERNAL_MOD");
			cross_bus -> internal_bus(*bus);
			cross_bus -> dram_if(*dram);
			cross_bus -> internal_clk(int_clk);
			cross_bus -> external_clk(ext_clk);

			eie_cc = new EIE_central_control("EIE_CENTRAL_CONTROL");
            eie_cc -> clk(int_clk);
			eie_cc -> bus_master(*bus);
			eie_cc -> bus_minion(*bus);

			for (int i = 0; i < NUM_ACCELERATORS; i++) {
				std::string name("EIE_ACCELERATOR_" + std::to_string(i));
				
				eie_accels[i] = new EIE_accelerator(name.c_str());
				eie_accels[i] -> clk(int_clk);

				eie_cc -> accelerators[i](*eie_accels[i]);
			}
			
			//Define the testbench thread
			// SC_THREAD(testbench);
			// 	dont_initialize();
			// 	sensitive << int_clk.pos();
		}
		
		void init_print(){
			cout << "\n---------------------------\n";
			cout << "\nStarting Project Simulation\n";
			cout << "\n---------------------------\n";
		}
	
		void final_print(int sw_c, int hw_c){
			cout << "\n----------------------------------\n";
			cout << "\nDONE Project Simulation\n";
			cout << "\nAT TIME: " << sc_time_stamp() << endl;
			cout << "\nSOFTWARE MODULE USED: " << sw_c << " cycles\n";
			cout << "\nHARDWARE MODULE USED: " << hw_c << " cycles\n";
			cout << "\n----------------------------------\n";
		}
}; //End module project_top

void print_help(){
	cout << "Project Usage: ./Proj_exec <-h> <-v>" << endl;
	cout << "    For help    : ./Proj_exec -h" << endl;
	cout << "    For verbose : ./Proj_exec -v" << endl;
}

int sc_main(int argc, char* argv[]){
	bool user_verbosity = false;
	if(argc > 1){
		std::string arg1 = std::string(argv[1]);
		if(arg1 == "-h" || arg1 == "--help"){
			print_help();
			exit(EXIT_FAILURE);
		}else if(arg1 == "-v" || arg1 == "--verbose"){
			user_verbosity = true;
		}else{
			print_help();
			exit(EXIT_FAILURE);
		}
	}
	
	sc_clock internal_clock ("internal_clock", clock_period_int, SC_NS);  
	sc_clock external_clock ("exernal_clock", clock_period_ex, SC_NS);  
	
	project_top top("top", user_verbosity);
	top.int_clk(internal_clock);
	top.ext_clk(external_clock);
	
	sc_start();
	
	return 0;
}
