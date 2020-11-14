#include "systemc.h"
#include "a2_include.h"
#include <stdio.h>
#include <stdlib.h> 

/*good output should be
[[  0,   0,   0,   0,   0,   0],
[  0, 162, 222, 219, 207, 218],
[  0, 284, 474, 416, 443, 483],
[  0,  76, 129, 115, 111, 139],
[  0,  48, 101,  83,  95, 101],
[  0, 130, 209, 199, 196, 220]]

real output
C = {0, 0, 0, 0, 0, 0,
     0, 162, 222, 219, 207, 218,
	 0, 284, 474, 416, 443, 483,
	 0, 76, 129, 115, 111, 139,
	 0, 48, 101, 83, 95, 101,
	 0, 130, 209, 199, 196, 220}

//this works
C = {0, 0, 0, 0, 0, 0, 0, 162, 222, 219, 207, 218, 0, 284, 474, 416, 443, 483, 0, 76, 129, 115, 111, 139, 0, 48, 101, 83, 95, 101, 0, 130, 209, 199, 196, 220 }
*/

class HW_module : public sc_module{
	
	private:
		sc_event event_HW_MST_COMPUTER;
		unsigned int current_status;
		unsigned int register_a[ARRAY_SIZE];
		unsigned int register_b[ARRAY_SIZE];
		unsigned int register_c[ARRAY_SIZE];
		bool print_output;
	
	public:
		sc_port<bus_master_if> bus_master;
		sc_port<bus_minion_if> bus_minion;
		
		sc_in<bool> clk;
		sc_out<int> master_cycles;
		
		SC_HAS_PROCESS(HW_module);
		
		HW_module(sc_module_name name, bool verbose) : sc_module(name) {
			cout << "HW MOD has been instantiated with name *" << name << "*\n";
			
			//initialize the registers
			for(int i = 0; i < ARRAY_SIZE; i++){
				register_a[i] = 0;
				register_b[i] = 0;
				register_c[i] = 0;
			}
			print_output = verbose;
			//bus master thread
			SC_THREAD(compute_hardware); 
				sensitive << clk.pos();
			
			//bus minion thread
			SC_THREAD(hardware_minion); 
				sensitive << clk.pos();
			
		}
	
		//ADDR_HW_MINION
		void hardware_minion(){
			unsigned int req_addr, req_op, req_len, rdata, wdata;
			wait(); //first wait is for the initialization
			while(true){
				//When Listen() returns, we can check if this call is for our module
				bus_minion->Listen(req_addr, req_op, req_len);
				//cout << "HW HELLOOOOO\n";
				//This request is for the memory, check the operation and size:
				if(req_addr == ADDR_HW_MINION){
					bus_minion->Acknowledge(); //ack the request, then fulfil the operation
					if(req_op == OP_HW_STAT){
						//SW MODULE WANTS TO READ STATUS
						wait(); //wait for the positive clock edge so that we follow bus protocols
						rdata = current_status;
						bus_minion->SendReadData(rdata);
					} else if (req_op == OP_HW_MUL){
						//SW MODULE WANTS TO DO MATRIX MULTIPLY
						if(current_status != STATUS_IP){
							//Since the module is not currently working on a hardware multiply allow the operation
							event_HW_MST_COMPUTER.notify(); //start the hardware multiply
						}
					} else {
						//invalid
						cout << "ERROR: INVALID OPERATION IN HW MODULE!\n";
					}
					
				}
			}
		}
		
		//ADDR_HW_MASTER
		void compute_hardware(){
			master_cycles.write(0);
			unsigned int rdata = 0;
			current_status = STATUS_DONE;
			
			//for tracking the performance
			int num_reads = 0;
			int num_writes = 0;
			int num_requests = 0;
			int num_acks = 0;
			
			while(true){
				wait(event_HW_MST_COMPUTER); //wait for the SW module to request a multiply
				if(print_output)
					cout << "HW DOING MAT MUL\n";
				current_status = STATUS_IP;
				
				//Need to request the matrix A 
				bus_master->Request(BUS_MST_HW, ADDR_A, OP_READ, ARRAY_SIZE);
				bus_master->WaitForAcknowledge(BUS_MST_HW);
				if(print_output)
					cout << "TIME " << sc_time_stamp() << ", HW_MOD done waiting\n";

				/** <THIS THREAD IS BUS MASTER> **/
				for(int i = 0; i < ARRAY_SIZE; i++){
					bus_master->ReadData(rdata);
					register_a[i] = rdata;
					//cout << "TIME " << sc_time_stamp() << ", HW_MOD read "  << rdata << " from bus\n";
				}
				/** </THIS THREAD IS BUS MASTER> **/
				//wait();
				//Need to request the matrix B 
				bus_master->Request(BUS_MST_HW, ADDR_B, OP_READ, ARRAY_SIZE);
				bus_master->WaitForAcknowledge(BUS_MST_HW);
				if(print_output)
					cout << "TIME " << sc_time_stamp() << ", HW_MOD done waiting\n";

				/** <THIS THREAD IS BUS MASTER> **/
				for(int i = 0; i < ARRAY_SIZE; i++){
					bus_master->ReadData(rdata);
					register_b[i] = rdata;
					//cout << "TIME " << sc_time_stamp() << ", HW_MOD read "  << rdata << " from bus\n";
				}
				/** </THIS THREAD IS BUS MASTER> **/
				
				if(print_output)
					cout << "TIME " << sc_time_stamp() << ", HW_MOD starting matrix multiply\n";
				for(int i = 0; i < 6; i++){
					for(int j = 0; j < 6; j++){
						register_c[i*6 + j] = 0; //reset the array value just in case. 
						for(int k = 0; k < 6; k++){
							//every 6 ks is a new element. each element of c is calculated by
							//multiplying 6 a's by 6 b's
							register_c[i*6 + j] += register_a[i*6 + k] * register_b[j + k*6];
						}
					}
				}
				
				if(print_output){
					cout << "C = {";
					for(int i = 0; i < ARRAY_SIZE; i++){
						cout << register_c[i] << ", ";
					}
					cout << "}\n";
				}
				wait();
				if(print_output)
					cout << "TIME " << sc_time_stamp() << ", HW_MOD done matrix multiply\n";
				
				//Need to request the matrix C for write back 
				bus_master->Request(BUS_MST_HW, ADDR_C, OP_WRITE, ARRAY_SIZE);
				bus_master->WaitForAcknowledge(BUS_MST_HW);
				if(print_output)
					cout << "TIME " << sc_time_stamp() << ", HW_MOD done waiting\n";

				/** <THIS THREAD IS BUS MASTER> **/
				for(int i = 0; i < ARRAY_SIZE; i++){
					//wait(); //for synchronization purposes. 
					bus_master->WriteData(register_c[i]);
					//cout << "TIME " << sc_time_stamp() << ", HW_MOD read "  << rdata << " from bus\n";
				}
				/** </THIS THREAD IS BUS MASTER> **/
				
				num_reads += 2*ARRAY_SIZE; //only read the bus twice, once for vector a, next for vector b.
				num_writes += ARRAY_SIZE; //only write the bus once, when writing back c.
				num_acks += 3; //for the acks
				num_requests += 3; //for the requests
				
				master_cycles.write(num_requests*2 + num_reads*2 + num_writes + num_acks); //writes take 1 clock cycle, reads 2, requests 2, ack 1
				current_status = STATUS_DONE; //done-zo
			}
			
			
		} //end compute matrices
	
	
};



