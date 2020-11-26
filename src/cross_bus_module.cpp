#include "systemc.h"
#include "project_include.h"
#include <stdio.h>
#include <stdlib.h> 

/**
This is the interface between the internal and external bus. 

The only device connected to the external bus is the DRAM. This module
is expected to act as a minion on the internal bus, but as THE master on 
the external bus. 

Initially, this module will only rd/wr single words from main memory (DRAM).
Later I will add implementation for a cache and necessarily also for reading
entire rows from DRAM. This will serve to populate the cache.

The implementation is all memory-mapped, so I don't have to worry about 
changing between internal and external addresses for I/O.
**/

class Cross_Bus : public sc_module {
	
	private:
		bool in_use;
		
		//Defining some events for synchronization
		sc_event dram_access_event;
		sc_event dram_done_event;
		
		//local variables
		unsigned int req_addr;
		unsigned int req_op;
		unsigned int req_len;
		unsigned int rdata;
		unsigned int wdata;
		
	public:
		//This is a bus minion, must connect to the minion port. 
		sc_port<bus_minion_if> internal_bus;
		sc_port<simple_mem_if> dram_if;
		
		sc_in<bool> internal_clk;
		sc_in<bool> external_clk;
		
		SC_HAS_PROCESS(Cross_Bus);
		
		//Constructor
		Cross_Bus(sc_module_name name) : sc_module(name) {
			cout << "CROSS BUS has been instantiated with name *" << name << endl;
			
			in_use = false;
			
			SC_THREAD(internal_bus_thread);
				sensitive << internal_clk.pos();
			
			SC_THREAD(external_bus_thread);
				sensitive << external_clk.pos();
		}
		
		/*
		This thread handles the internal bus interface. It waits until the external address has
		been called, then signals the external thread to access the DRAM. 
		*/
		void internal_bus_thread(){
			wait(); //first wait is for the initialization
			while(true){
				//When Listen() returns, we can check if this call is for our module
				internal_bus->Listen(req_addr, req_op, req_len);
				
				//This request is for the memory, check the operation and size:
				if(req_addr >= DRAM_START_ADDR && req_addr < DRAM_START_ADDR + DRAM_SIZE){
					internal_bus->Acknowledge(); //ack the request, then fulfil the operation
					
					for(unsigned int i = 0; i < req_len; i++){
						if(req_op == OP_READ){
							//read operation
							wait(); //wait for the positive clock edge so that we follow bus protocols
							dram_access_event.notify();
							wait(dram_done_event);
							internal_bus->SendReadData(rdata);
						} else if (req_op == OP_WRITE){
							//write
							internal_bus->ReceiveWriteData(wdata);
							dram_access_event.notify();
							wait(dram_done_event);
						} else {
							//invalid
							cout << "ERROR: INVALID OPERATION IN CROSS_BUS MODULE!\n";
						}
					}
				}
			}
			
		}
		
		/*
		This thread handles the interconnection with the DRAM at the DRAM clock speed. 
		
		When the internal bus signals that there is a request incoming, this module will retrieve the 
		desired information from the DRAM and output it here. 
		*/
		void external_bus_thread(){
			wait();
			while(true){
				wait(dram_access_event);
				//cout << "TIME " << sc_time_stamp() << ", ACCESSING DRAM!\n";
				if(req_op == OP_READ){
					//read
					dram_if->Read(req_addr, rdata);
				} else{
					//write
					dram_if->Write(req_addr, wdata);
				}
				dram_done_event.notify();
			}
		}
};
