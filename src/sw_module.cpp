#include "systemc.h"
#include "project_include.h"
#include <stdio.h>
#include <stdlib.h> 

/**
===========================================================================================
TODO:Change all comments and file names to match project
===========================================================================================
**/

/**
This class implements sc_module to carry out the requirements for the software portion
of the assignment. The software module computes one portion of the matrix multiplication
and leaves the rest to a hardware accelerator. The hardware accelerator implements the 
most computationally intensive operations, that is, all the software module does is:
	- set the matrix c to zero.
	- invocate the hardware accelerator.
	- read the resulting matrix from memory. 
**/

class SW_module : public sc_module{
	
	private:
		bool print_output;
	public:
		sc_port<bus_master_if> bus;
		
		sc_in<bool> clk;
		sc_in<bool> get_multiply_result;
		sc_in<bool> output;
		sc_out<bool> done_multiply;
		sc_out<int> get_cycles;
		
		SC_HAS_PROCESS(SW_module);
		
		SW_module(sc_module_name name, bool verbose) : sc_module(name) {
			cout << "SW MOD has been instantiated with name *" << name << "*\n";
			print_output = verbose;
			SC_THREAD(compute_matrices); 
				sensitive << clk.pos();
		}
	
		/*
		Implements software functionality for the matrix multiply. This consists of
		setting the array c to 0, invocing the hardware acclerator, then reading the
		result back. 
		*/
		void compute_matrices(){
			done_multiply.write(true);
			get_cycles.write(0);
			unsigned int rdata = 0;
			unsigned int wdata = 0;
			int len = SIZE_ARRAY;
			
			/*<TEST DRAM AND INTERNAL-EXTERNAL BUS FUNCTIONALITY>*/
			wait();
			bus->Request(BUS_MST_SW, DRAM_BASE_ADDR, OP_WRITE, 1);
			bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
			cout << "TIME " << sc_time_stamp() << ", SW_MOD trying to write to DRAM!\n";
			bus->WriteData(1234567);
			cout << "TIME " << sc_time_stamp() << ", SW_MOD finished write to DRAM!\n";
			bus->Request(BUS_MST_SW, DRAM_BASE_ADDR, OP_READ, 1);
			bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
			cout << "TIME " << sc_time_stamp() << ", SW_MOD trying to read from DRAM!\n";
			bus->ReadData(rdata);
			cout << "TIME " << sc_time_stamp() << ", DATA READ BACK FROM DRAM: " << rdata << endl;
			cout << "TIME " << sc_time_stamp() << output.read() << endl;
			/*</TEST DRAM AND INTERNAL-EXTERNAL BUS FUNCTIONALITY>*/
			
			//for tracking the performance
			int num_reads = 0;
			int num_writes = 0;
			int num_requests = 0;
			int num_acks = 0;
			while(true){
				wait(); //wait for clock and mem request command
				
				//if the matrix multiply command has come in, start the process for completing
				//the operation. 
				if(get_multiply_result.read() == true){
					done_multiply.write(false);
					//REQUEST BUS MASTER
					//request burst memory access to write initial values to matrix c
					bus->Request(BUS_MST_SW, ADDR_C, OP_WRITE, SIZE_ARRAY);
					bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
					
					if(print_output)
						cout << "TIME " << sc_time_stamp() << ", SW_MOD HAS BUS MASTER!\n";
					
					/** <THIS THREAD IS BUS MASTER> **/
					for(int i = 0; i < SIZE_ARRAY; i++){
						//wait();
						bus->WriteData(wdata);
						//cout << "TIME " << sc_time_stamp() << ", SW WRITE!\n";
					}
					/** </THIS THREAD IS BUS MASTER> **/
					
					if(print_output)
						cout << "TIME " << sc_time_stamp() << ", SW_MOD HAS FINISHED WRITE OPERATION. RELINQUISH BUS!\n";
					
					
					
					//request hw acceleration
					bus->Request(BUS_MST_SW, ADDR_HW_MINION, OP_HW_MUL, 0);
					bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
					
					if(print_output)
						cout << "TIME " << sc_time_stamp() << ", SW_MOD HAS BUS MASTER!\n";
					
					/** <THIS THREAD IS BUS MASTER> **/
					//don't need to do anything..
					/** </THIS THREAD IS BUS MASTER> **/
					
					
					do{
						//request hw status
						bus->Request(BUS_MST_SW, ADDR_HW_MINION, OP_HW_STAT, 1);
						bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
						bus->ReadData(rdata);
						if(rdata == STATUS_IP){
							if(print_output)
								cout << "TIME " << sc_time_stamp() << ", SW_MOD checked HW status, still in progress\n";
						} else {
							if(print_output)
								cout << "TIME " << sc_time_stamp() << ", SW_MOD checked HW status, finished acceleration\n";
						}
						num_requests++;
						num_reads++;
						num_acks++;
					} while(rdata == STATUS_IP);
					
					//request burst memory access to read from matrix c
					bus->Request(BUS_MST_SW, ADDR_C, OP_READ, SIZE_ARRAY);
					bus->WaitForAcknowledge(BUS_MST_SW); //wait for bus response
					
					if(print_output)
						cout << "TIME " << sc_time_stamp() << ", SW_MOD HAS BUS MASTER!\n";
					
					/** <THIS THREAD IS BUS MASTER> **/
					for(int i = 0; i < SIZE_ARRAY; i++){
						bus->ReadData(rdata);
						if(output.read()){
							cout << "TIME " << sc_time_stamp() << ", SW_MOD read output matrix value: "  << rdata << " from bus\n";
						}
					}
					/** </THIS THREAD IS BUS MASTER> **/
					
					num_requests += 3; //for the main 3 requests to write c, request HW, and read c
					num_acks += 3;
					num_reads += SIZE_ARRAY; //for the final read operation
					num_writes += SIZE_ARRAY; //for writing c at the start
				}
				
				get_cycles.write(num_requests*2 + num_reads*2 + num_writes + num_acks); //writes take 1 clock cycle, reads 2, requests 2, ack 1
				done_multiply.write(true);
			}
			
			
		} //end compute matrices
	
	
};


