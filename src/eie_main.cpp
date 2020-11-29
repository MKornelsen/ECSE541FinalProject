/*************************************************************
EIE_Main.cpp is the main file for the EIE TLM model. This 
model takes into account power modelling and system-level 
modelling to produce a system suitable for design space 
exploration. The system consists of a CPU (eie_sw_module),
an accelerator control unit (eie_central_control), the 
accelerators themselves (eie_accelerator), an internal-to-
external bus module (cross_bus_module), and finally a DRAM 
module. Power modelling is done with some approximations 
based on the reported energy usage in the EIE paper. 
*************************************************************/

#include <systemc.h>
#include <project_include.h>
#include "bus.h"
#include "cross_bus_module.cpp"
#include "DRAM.cpp"
#include "eie_accelerator.h"
#include "eie_sw_module.h"

#define help_usage 1
#define help_file 2

#define clock_period_int 0.5
#define clock_period_ex  20

//Define the power numbers - all numbers in pJ
#define POWER_FL_ADD      0.9
#define POWER_FL_MUL      3.7
#define POWER_INT_ADD     0.1
#define POWER_INT_MUL     3.1
#define POWER_SRAM        5.0
#define POWER_ACC_BUS     5.5
#define POWER_BUS         1.0
#define POWER_REGISTER    1.0
#define POWER_DRAM        640.0

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
		
		//Static and dynamic power estimates tallied from the modules
		double power_dynamic, power_static;
		
		SC_HAS_PROCESS(project_top);
		
		//Top module constructor
		project_top(sc_module_name name, bool verbose) : sc_module(name) {
			init_print();
			
			power_dynamic = 0; //sum and multiple of tallies with energy numbers.
			power_static = 0; //0.1 times the dynamic power
			
			//Define the tracker thread -> NEEDS TO BE DONE BEFORE THE REST OF THE MODULES
			SC_THREAD(event_tracker);
			 	sensitive << int_clk.pos();
			
			//Instantiate the objects and link them to the various ports and signals
			bus = new bus_clocked("MY_BUS");
			bus->clk(int_clk);
			unsigned int idtmp;
			bus->attach_master(idtmp);
			bus->attach_master(idtmp);

            eie_sw = new EIE_SW_module("EIE_SW");
            eie_sw -> bus(*bus);
			
			dram = new DRAM("MY_DRAM");
			dram -> clk(ext_clk);
			
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
			
		}
		
		void event_tracker(){
			//See when EIE_SW is done loading weights
			cout << "HELLO???\n";
			wait(eie_sw->done_execution);
			
			//DRAM accesses from CPU (expected due to instruction loading) and the cross_bus tally
			int total_dram_tally = eie_sw->tally_dram_access + cross_bus->transfer_tally;
			
			//SRAM accesses and float operations from EIE_ACC only
			int sram_tally = 0;
			int float_add_tally = 0;
			int float_mult_tally = 0;
			
			for (int i = 0; i < NUM_ACCELERATORS; i++) {
				sram_tally += eie_accels[i] -> tally_sram_access;
				float_add_tally += eie_accels[i] -> tally_float_add;
				float_mult_tally += eie_accels[i] -> tally_float_multiply;
			}
			
			int tally_cc_bus = eie_cc->tally_transfers_acc_bus;
			
			int tally_bus = bus->tally_bus_transfers;
			
			int tally_cc_register = eie_cc->tally_output_read;
			
			int int_add_tally = eie_sw->tally_int_add;
			int int_mult_tally = eie_sw->tally_int_multiply;
			
			double power_float_ops = POWER_FL_ADD*float_add_tally + POWER_FL_MUL*float_mult_tally;
			double power_int_ops   = POWER_INT_ADD*int_add_tally + POWER_INT_MUL*int_mult_tally;
			double power_sram      = POWER_SRAM*sram_tally;
			double power_acc_bus   = POWER_ACC_BUS*tally_cc_bus;
			double power_bus       = POWER_BUS*tally_bus;
			double power_dram      = POWER_DRAM*total_dram_tally;
			double power_register  = POWER_REGISTER*tally_cc_register;
			
			double total_power = power_float_ops + power_int_ops + power_sram + power_acc_bus + power_bus + power_dram;
			
			cout << "\n----------------------------------\n";
			cout << "\nDONE Project Simulation\n";
			cout << "\nAT TIME: " << sc_time_stamp() << endl << endl;
			cout << "Power from integer operations = " << power_int_ops << " pJ\n";
			cout << "Power from float operations = "   << power_float_ops << " pJ\n";
			cout << "Power from SRAM accesses = "      << power_sram << " pJ\n";
			cout << "Power from accelerator bus = "    << power_acc_bus << " pJ\n";
			cout << "Power from internal bus = "       << power_bus << " pJ\n";
			cout << "Power from DRAM accesses = "      << power_dram << " pJ\n";
			cout << "Power from register accesses = "  << power_register << " pJ\n";
			cout << "\n----------------------------------\n";
			cout << "\nTotal power = " << total_power << " pJ\n";
			cout << "\n----------------------------------\n";
			
			sc_stop();
		}
		
		void init_print(){
			cout << "\n---------------------------\n";
			cout << "\nStarting Project Simulation\n";
			cout << "\n---------------------------\n";
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
