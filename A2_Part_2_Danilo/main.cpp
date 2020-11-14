#include "systemc.h"
#include "a2_include.h"
#include "bus.cpp"
#include "sw_module.cpp"
#include "hw_module.cpp"
#include "memory.cpp"
#include <stdio.h>
#include <stdlib.h> 

#define help_usage 1
#define help_file 2

/**
This is the top-module and main file for the system-level design of a
HW-SW system with memory and an arbitrated bus to perform matrix 
multiplication. This is the first part of the assignment, so this 
model is (or rather is supposed to be) un-timed. I did not make the 
model completely untimed since I used concepts like wait() and events
so that control could be passed between threads. This means that the 
implementation inherently has some timing concepts entagled, but it does
not implement delays as would be expected for part 2 of the assignment. 

Author: Danilo Vucetic
Date: October 24, 2020
**/

#define clock_period 6.666

//Top module
class a2_top_part2 : public sc_module {
	
	private:
		
	public:
		//Signals and ports
		sc_in<bool> clk;
		sc_signal<bool> req_1;
		sc_signal<bool> done, show_output;
		sc_signal<int> sw_cycles, hw_cycles;
		
		//Object references
		Bus * my_bus;
		SW_module * my_sw;
		HW_module * my_hw;
		Memory    * my_mem;
		
		SC_HAS_PROCESS(a2_top_part2);
		
		//Top module constructor
		a2_top_part2(sc_module_name name, bool verbose) : sc_module(name) {
			init_print();
			
			//Instantiate the objects and link them to the various ports and signals
			my_bus = new Bus("MY_BUS", verbose);
			
			my_sw = new SW_module("MY_SW", verbose);
			my_sw -> clk(clk);
			my_sw -> get_multiply_result(req_1);
			my_sw -> done_multiply(done);
			my_sw -> output(show_output);
			my_sw -> get_cycles(sw_cycles);
			my_sw -> bus(*my_bus);
			
			my_hw = new HW_module("MY_HW", verbose);
			my_hw -> clk(clk);
			my_hw -> bus_master(*my_bus);
			my_hw -> bus_minion(*my_bus);
			my_hw -> master_cycles(hw_cycles);
			
			my_mem = new Memory("MY_MEM");
			my_mem -> clk(clk);
			my_mem -> bus(*my_bus);
			
			//Define the testbench thread
			SC_THREAD(testbench);
				dont_initialize();
				sensitive << clk.pos();
		}
		
		//This is just a simple testbench to run the system
		void testbench(){
			req_1.write(false);
			show_output.write(true);
			wait(clock_period, SC_NS); //allow the signal to be registered
			
			for(int i = 0; i < 1000; i++){
				req_1.write(true);
				wait(clock_period, SC_NS);
				req_1.write(false);
				wait(clock_period, SC_NS);

				while(done.read() != true){
					wait(clock_period, SC_NS);
				}
				
				wait(clock_period, SC_NS);
				show_output.write(false);
			}
			
			int sw_c = sw_cycles.read();
			int hw_c = hw_cycles.read();
			
			final_print(sw_c, hw_c);
			sc_stop();
		}
	
		void init_print(){
			cout << "\n---------------------------------------\n";
			cout << "\nStarting Assignment 2 Part 2 Simulation\n";
			cout << "\n---------------------------------------\n";
		}
	
		void final_print(int sw_c, int hw_c){
			cout << "\n----------------------------------\n";
			cout << "\nDONE Assignment 2 Part 2 Simulation\n";
			cout << "\nAT TIME: " << sc_time_stamp() << endl;
			cout << "\nSOFTWARE MODULE USED: " << sw_c << " cycles\n";
			cout << "\nHARDWARE MODULE USED: " << hw_c << " cycles\n";
			cout << "\n----------------------------------\n";
		}
}; //End module a2_top_part2

void print_help(){
	cout << "Assignment 2 Part 2 Usage: ./A2_P2_exec <-h> <-v>" << endl;
	cout << "    For help    : ./A2_P2_exec -h" << endl;
	cout << "    For verbose : ./A2_P2_exec -v" << endl;
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
	
	sc_clock clock ("my_clock", 6.666, SC_NS); //roughly 150 MHz 
	
	a2_top_part2 top("top", user_verbosity);
	top.clk(clock);
	
	sc_start();
	
	return 0;
}
