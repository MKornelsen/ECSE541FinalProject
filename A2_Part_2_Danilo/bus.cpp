#include "systemc.h"
#include "a2_include.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

/**
This is the arbitrated bus component for assignment 2 of ECSE 541.

The arbitrated bus implements sc_module, bus_master_if, and bus_minion_if.
These interfaces are specifically used since the component must be a SystemC
module, and it also must handle all bus transactions. Transaction-level modelling
is used to represent the bus and all its functions. This means that instead of 
running a truly hardware-oriented bus, I will write a bus that works at an 
abstracted level. This includes handling of requests, blocking on certain
actions, etc. 
**/

class Bus : public sc_channel, public bus_master_if, public bus_minion_if{
	
	private:
		//private information to be stored. 
		request_t *current; //These are the request data structures.
		request_t *temp;
	
		//These are the events I use to synchronize the threads
		sc_event event_ack_min_mas;
		sc_event event_req_mas;
		sc_event event_req_release;
		sc_event event_data_read;
		sc_event event_data_read_ack;
		sc_event event_data_write;
		sc_event event_data_write_ack;
		sc_event event_unlock_mutex;
		
		//variables for data in transit
		unsigned int wdata_transit;
		unsigned int rdata_transit;
		
		//formal sc_mutex defines to make mutual exclusion simpler
		sc_mutex mutex_lock_req_func;
		sc_mutex mutex_lock_ack_func;
		
		//verbosity indicator
		bool print_output;
		
	
	public:
		//the publicly accesible functions and data
		
		SC_HAS_PROCESS(Bus);
		
		//Constructor
		Bus(sc_module_name name, bool verbose) : sc_channel(name) {
			cout << "BUS has been instantiated with name *" << name << "*\n";
			
			//initialize some variables and structs
			current = NULL;
			temp = NULL;
			wdata_transit = 0;
			rdata_transit = 0;
			print_output = verbose;
		}
		
		void delete_request(){
			while(mutex_lock_req_func.trylock() == -1){
				wait(event_req_release);
			}
			if(print_output)
				cout << "TIME " << sc_time_stamp() << "; REQUEST DELETED: " << current->mst_id << "op == " << current->op << endl;
			
			temp = current;
			current = current->next;
			delete temp;
			temp = NULL;
			
			//unlock the mutex and notify all waiting threads
			mutex_lock_req_func.unlock();
			event_req_release.notify();
		}
		
		/** <Bus master functions> **/
		
		/* The Request() function creates (mallocs) a new request_t data structure (see 
		a2_include.h for details). The request structure is considered shared memory and
		so this function has a mutex lock preventing multiple access. 
		*/
		void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len){
			//For each thread the enters this function, concurrently or otherwise, it must 
			//aquire the mutex lock to access this function. If it does not, then it will 
			//have to wait for the event 'event_req_release' to try again. 
			while(mutex_lock_req_func.trylock() == -1){
				wait(event_req_release);
			}
			wait(); wait();
			if(print_output)
				cout << "TIME " << sc_time_stamp() << "; REQ: " << mst_id << "op == " << op << endl;
			
			//Create the request
			temp = new request_t;
			temp->mst_id = mst_id;
			temp->addr = addr;
			temp->op = op;
			temp->len = len;
			temp->next = NULL; //there is no next node yet
			
			if(current == NULL){
				//no current requests, just add this to the front. 
				current = temp;
			}else{
				//Since there is already a request potentially in progress, we must check if the ordering
				//of requests fits the round robin schedule. I take it to just mean that the requests for 
				//one master alternate with the requests of another. Since a request in progress can not
				//be interrupted, I will just try to reorder nodes based on master id.
				bool done_walking = false;
				request_t *temp2 = current;
				
				/*
				The following is the round-robin scheduling code. It's pretty messy but it works as follows:
				
				Since I can't really define some time allocation for each request (it is tremedously difficult
				to keep track of the number of operations thusfar undertaken and interrupt a hardware device, 
				plus, I haven't seen those types of arbitration schemes), I will instead just try to give each 
				master an equal number of requests on the bus. This means that the masters will alternate in 
				requests. I implement this by placing the requests ordered by master ID, and if there are 2 
				equal master id's in a row, I put a non-similar id between them. Since this process takes place
				as I'm building the list, I can be sure that the list will always be alternating. 
				*/
				do{
					if(temp2->next != NULL){
						if(temp2->mst_id == temp2->next->mst_id){
							if(temp2->mst_id == mst_id){
								//continue walking until the first discrepency arises. 
								temp2 = temp2->next;
							}else{
								//there is a discrepency, insert the different id 
								temp->next = temp2->next;
								temp2->next = temp;
								done_walking = true;
							}
						} else { 
							//the two nodes do not match. 
							//In this case we just keep walking until we either hit the end, or find discrepency
							temp2 = temp2->next;
						}
					} else {
						done_walking = true;
						temp2->next = temp; //insert temp at the end of the list
					}
				}while(!done_walking);
				
				if(current->mst_id != mst_id){
					
					
					//since the current master is not the same as the requesting, put the requesting next in line
					temp->next = current->next; //save the former next request
					current->next = temp;
				} else {
					//here we have to check a few things.
					if(current->next != NULL){
						if(current->mst_id == mst_id && current->next->mst_id == mst_id){
							//if there are two of the same master ids in a row, put the 
						}
					}
				}
				
			}
			temp = NULL;
			
			//unlock the mutex and notify all waiting threads
			mutex_lock_req_func.unlock();
			event_req_release.notify();
		}
		
		/* Since the Request function is more or less just waiting for a call and then
		creating the request, in this function we can assume that the request has already 
		occured, otherwise the thread will just be forever blocked. Hence, in this function
		the request event event_req_mas must be notified to tell the minions that a request
		can be fulfilled. When a minion returns from Listen(), they will immediately call
		Acknowledge(), and the desired thread will be released to master the bus.
		
		The mutex implementation with sc_mutex is much more efficient than my previous 
		attempt with boolean variables and events. 
		*/
		bool WaitForAcknowledge(unsigned int mst_id){
			while(true){
				if(mutex_lock_ack_func.trylock() != -1) {
					//check if the current execution context is the correct one
					if(current->mst_id == mst_id){
						wait(); //Ack takes one clock cycle (at least)
						//cout << "ACK: " << mst_id << endl;
						//if the current execution context is the correct thread, return true
						//and allow the thread to master the bus.
					
						event_req_mas.notify(); //notify the minions of a request. 
						if(print_output)
							cout << "TIME " << sc_time_stamp() << "; MASTER: " << current->mst_id << " waiting on ack\n";
						//wait for the acknowledge to occur
						wait(event_ack_min_mas);
						if(print_output)
							cout << "TIME " << sc_time_stamp() << "; MASTER: " << current->mst_id << " done wait on ack\n";
						if(current->len == 0){
							//this is just a notification not a memory access.
							if(print_output)
								cout << "MASTER: " << current->mst_id << " has finished 0 length operation\n";
							temp = current;
							current = current->next;
							delete temp;
							temp = NULL;
							mutex_lock_ack_func.unlock();
							event_unlock_mutex.notify();
						}else{
							mutex_lock_ack_func.unlock();
						}
						return true;
					} else {
						if(print_output)
							cout << "TIME " << sc_time_stamp() << "; LOCKED 1: " << mst_id << endl;
						mutex_lock_ack_func.unlock();
						wait(event_unlock_mutex);
					}
				} else {
					if(print_output)
						cout << "TIME " << sc_time_stamp() << "; LOCKED 2: " << mst_id << endl;
					wait(event_unlock_mutex);
				}
			}
		}
		
		/* OLD WORKING CODE for WaitForAcknowledge()
		============================
		THIS IS ADDED JUST FOR REFERENCE, TO SAY THAT IT CAN BE DONE THIS WAY, BUT
		YOU SHOULD PROBABLY JUST USE MUTEX INSTEAD.
		============================
		if(lock_m){
			cout << "TIME " << sc_time_stamp() << "; LOCKED 1: " << mst_id << endl;
			wait(event_unlock_mutex);
		}else{
			lock_m = true;
			event_req_mas.notify(); //notify the minions of a request. 
			cout << "TIME " << sc_time_stamp() << "; MASTER: " << current->mst_id << " waiting on ack\n";
			//wait for the acknowledge to occur
			wait(event_ack_min_mas);
			cout << "TIME " << sc_time_stamp() << "; MASTER: " << current->mst_id << " done wait on ack\n";
			//check if the current execution context is the correct one
			if(current->mst_id == mst_id){
				//cout << "ACK: " << mst_id << endl;
				//if the current execution context is the correct thread, return true
				//and allow the thread to master the bus.
				if(current->len == 0){
					//this is just a notification not a memory access.
					cout << "MASTER: " << current->mst_id << " has finished 0 length operation\n";
					temp = current;
					current = current->next;
					delete temp;
					temp = NULL;
					event_unlock_mutex.notify();
					lock_m = false;
				}else{
					current->acknowledged = true;
					lock_m = true;
				}
				return true;
			}else{
				wait(event_unlock_mutex);
				cout << "TIME " << sc_time_stamp() << "; LOCKED 2: " << mst_id << endl;
			}
		}*/
		
		/* Implements the ReadData functionality, namely, it facilitates the reading of 
		the minions by the master device. It should be noted that while the master has
		the bus, no other threads can have concurrent access, which means that this entire
		thread is basically under a mutex lock (it doesn't really mean that, but from the
		perspective of a simple bus protocol we're safe to make the assumption.)
		*/
		void ReadData(unsigned int &data){
			wait(event_data_read);
			wait();wait(); //read operation takes at least 2 clock cycles
			data = rdata_transit;
			event_data_read_ack.notify();
			
			//decrement len, if zero, then request is over.
			current->len--;
			if(current->len == 0){
				//cout << "MASTER: " << current->mst_id << " has finished reading\n";
				delete_request();
				event_unlock_mutex.notify();
			}
			//wait(); //read operation takes at least 2 clock cycles
		}
		
		/* Implements WriteData functionality, namely, it facilitates the writing of
		data to the minion devices by the master device. See ReadData() note for details 
		of multi-threading. 
		*/
		void WriteData(unsigned int data){
			wait(); //a write operation takes one clock cycle
			wdata_transit = data;
			event_data_write.notify();
			wait(event_data_write_ack);
			wdata_transit = 0; //clear transit data on the bus.
			
			//decrement len, if zero, then request is over.
			current->len--;
			if(current->len == 0){
				//cout << "MASTER: " << current->mst_id << " has finished writing\n";
				delete_request();
				event_unlock_mutex.notify();
			}
		}
		
		/** </Bus master functions> **/
		
		/** <Bus minion functions> **/
		
		/* This function is where the bus minions will wait for the requests to come in from
		the masters. The master will pass the information to the Request function, but the
		minions will be waiting on the Listen function. After returning from Listen(), 
		the minions are expected the call Acknowledge(). It is assumed that only one minion at
		a time will be able to do this, and that all minions return from Listen() at the same
		time. This function does not need a mutex lock because there is no shared memory 
		between threads: each thread will have it's own execution context (in classic C this 
		is called the frame I believe) which is private to the thread. Only in situations where
		shared data is accessed (namely when it is being written) do we need a mutex. Notice 
		that if 'current' was written to, this function would be subject to error, but since
		we made the assumption that only a single master can hold the bus at a time, we can 
		also assume that the 'current' variable will not be altered. 
		*/
		void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len){
			//NOTE: no while loop here because the minion should keep track of all potential requests. 
			//wait for the request to occur
			wait(event_req_mas);

			//Return data for the minions to analyze to see if this is for them.
			req_addr = current->addr;
			req_op   = current->op;
			req_len  = current->len;
		}
		
		/* Minions call Acknowledge() when they check their addresses and realize they are
		being accessed by a request. 
		*/
		void Acknowledge(){
			//notify the current master that the requested hardware is available
			event_ack_min_mas.notify();
		}
	
		/* This function is called by the minion when it has something to return.
		I.e. the minion can return a memory value to the bus. This will only be 
		called on a valid request. 
		*/
		void SendReadData(unsigned int data){
			rdata_transit = data;
			event_data_read.notify();
			wait(event_data_read_ack);
			rdata_transit = 0; //clear transit data on the bus.
		}
		
		/* This function is called by the minion when it has the ability to write 
		data to the requested address. 		
		*/
		void ReceiveWriteData(unsigned int &data){
			wait(event_data_write);
			data = wdata_transit;
			event_data_write_ack.notify();
		}
		
		/** </Bus minion functions> **/
		
};

