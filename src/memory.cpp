#include "systemc.h"
#include "project_include.h"
#include <stdio.h>
#include <stdlib.h> 

/**
===========================================================================================
TODO:Change all comments and file names to match project
===========================================================================================
**/

class Memory : public sc_module {
	
	private:
		unsigned int memData[MEM_SIZE];
		//Memory values
		const unsigned int a[ARRAY_SIZE] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
		const unsigned int b[ARRAY_SIZE] = {0,0,0,0,0,0,0,0,9,4,7,9,0,12,14,15,16,11,0,2,3,4,5,6,0,4,3,2,1,2,0,2,7,6,4,9};
	
	public:
		sc_port<bus_minion_if> bus;
		
		sc_in<bool> clk;
		
		SC_HAS_PROCESS(Memory);
		
		//Constructor
		Memory(sc_module_name name) : sc_module(name) {
			cout << "MEMORY has been instantiated with name *" << name << endl;
			
			//initialize the memory
			for(int i = 0; i < MEM_SIZE; i++){
				if(i<ARRAY_SIZE){
					memData[i] = a[i];
				} else if(i < 2*ARRAY_SIZE){
					memData[i] = b[i-ARRAY_SIZE];
				} else {
					memData[i] = 0; //c is initialized to 0
				}
			}
			
			SC_THREAD(mem_thread);
				sensitive << clk.pos();
		}
		
		void mem_thread(){
			unsigned int req_addr, req_op, req_len, rdata, wdata;
			wait(); //first wait is for the initialization
			while(true){
				//When Listen() returns, we can check if this call is for our module
				bus->Listen(req_addr, req_op, req_len);
				
				//This request is for the memory, check the operation and size:
				if(req_addr >= DRAM_BASE_ADDR && req_addr < MEM_SIZE){
					bus->Acknowledge(); //ack the request, then fulfil the operation
					
					for(unsigned int i = 0; i < req_len; i++){
						
						//cout << "after wait 1 in mem\n";
						if(req_op == OP_READ){
							//read operation
							wait(); //wait for the positive clock edge so that we follow bus protocols
							rdata = memData[req_addr+i];
							bus->SendReadData(rdata);
						} else if (req_op == OP_WRITE){
							//write
							bus->ReceiveWriteData(wdata);
							memData[req_addr+i] = wdata;
						} else {
							//invalid
							cout << "ERROR: INVALID OPERATION IN MEMORY MODULE!\n";
						}
					}
				}
			}
		}
};

