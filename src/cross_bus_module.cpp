#include "systemc.h"
#include "project_include.h"
#include <stdio.h>
#include <stdlib.h> 

/*************************************************************
Cross_Bus_Module.h is the interface between the internal and 
external bus. The only device connected to the external bus is
the DRAM. This module is expected to act as a minion on the 
internal bus, but as THE master on the external bus. 

The implementation is all memory-mapped, so you don't have to 
worry about changing between internal and external addresses
for I/O.

Future considerations: 
	Adding cache to this module would serve to model a real 
	processor. Almost every processor these days comes with
	cache of some kind and if we're modelling an SoC without
	cache at some level it's kind of a disingenuous model. 

POWER MODELLING:
	Power modelling is carried out with Yousef's power 
	estimates which are based both on the EIE paper. Each DRAM
	access is modelled as taking 640 pJ due to the large load
	of both the external bus itself, but also the DRAM since
	it is a huge capacitive load. We assume that this number 
	also takes into account the inductive loads from pads etc.
	
	Each transfer will be modelled as a 32-bit transfer of 
	640 pJ and a running tally is kept of the number of 
	transfers. 
	
	It may seem strange that the power model for the DRAM is
	encapsulated in the bus connecting to it, but this is 
	because we do not have any numbers for the actual bus
	power versus the DRAM, we just have power numbers for a
	DRAM access in totality. 
*************************************************************/


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

		unsigned int dram_req_addr;
		
	public:
		//This is a bus minion, must connect to the minion port. 
		sc_port<bus_minion_if> internal_bus;
		sc_port<simple_mem_if> dram_if;
		
		sc_in_clk internal_clk;
		sc_in_clk external_clk;
		
		int transfer_tally;
		
		SC_HAS_PROCESS(Cross_Bus);
		
		//Constructor
		Cross_Bus(sc_module_name name) : sc_module(name) {
			cout << "CROSS BUS has been instantiated with name *" << name << endl;
			
			transfer_tally = 0;
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
				if(req_addr >= DRAM_BASE_ADDR && req_addr < DRAM_BASE_ADDR + DRAM_SIZE){
					internal_bus->Acknowledge(); //ack the request, then fulfil the operation
					
					for(unsigned int i = 0; i < req_len; i++){
						dram_req_addr = req_addr + i;
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
				transfer_tally += 1; //keep track of transfers.
				if(req_op == OP_READ){
					//read
					dram_if->Read(dram_req_addr, rdata);
				} else{
					//write
					dram_if->Write(dram_req_addr, wdata);
				}
				dram_done_event.notify();
			}
		}
};
