#include "systemc.h"
#include "project_include.h"
#include <iomanip>

/**
Class DRAM implements SC_MODULE and SIMPLE_MEM_IF. The latter interface is from
assignment 1 of 541 and allows for a relatively simple method for accessing a 
memory element. Since the DRAM is the only device on the external bus for our
project, there is no need for a complicated memory arbiter as is used for the
internal bus. 

The only caveat is that any module that is accessing the DRAM MUST BE A 
SC_THREAD! This is because the Read() and Write() functions need to have
wait() statements inside of them sensitive to the clock. Otherwise the 
module will wait forever without any triggers. 
**/
class DRAM : public sc_module, public simple_mem_if {
	
	private:
		unsigned int main_memory[DRAM_SIZE];
		char correctLabels[TEST_IMAGES];
	public:
		
		SC_HAS_PROCESS(DRAM);
		
		//Constructor
		DRAM(sc_module_name name) : sc_module(name) {
			cout << "DRAM has been instantiated with name *" << name << endl;

			//initialize the memory
			for(int i = 0; i < DRAM_SIZE; i++){
				main_memory[i] = 0;
			}
			
			unsigned int base_addr = 0;

			for (int i = 0; i < NUM_LAYERS; i++) {
				std::string path("Weights/");
				path += std::string("weight_l") + std::to_string(i) + std::string(".txt");
				cout << path << endl;
				
				ifstream input;
				input.open(path, ios::in);

				std::vector<unsigned int> tmp;
				float f;
				unsigned int ui;
				while (!input.eof()) {
					input >> f;
					ui = *(unsigned int *) &f;
					tmp.push_back(ui);
				}
				cout << "weight layer " << i << " size = " << tmp.size() << endl;
				input.close();
				
				for (int j = 0; j < tmp.size(); j++) {
					main_memory[base_addr + j] = tmp.at(j);
				}
				
				unsigned int layersizes[NUM_LAYERS + 1] = LAYER_SIZES;

				for (int j = 0; j < NUM_ACCELERATORS; j++) {
					
					cout << *(float *) &main_memory[base_addr + j * layersizes[i]] << " ";
					cout << *(float *) &main_memory[base_addr + j * layersizes[i] + 1] << " ";
					cout << *(float *) &main_memory[base_addr + j * layersizes[i] + 2] << " ";
					cout << endl;
				}
				
				base_addr += (unsigned int) tmp.size();
			}

			ifstream imgs, labels;
			imgs.open("MNIST/t10k-images.idx3-ubyte", ios::in | ios::binary);
			labels.open("MNIST/t10k-labels.idx1-ubyte", ios::in | ios::binary);

			char imgbuf[28 * 28];
			char labelbuf[8];

			imgs.read(imgbuf, 16);
			labels.read(labelbuf, 8);
			std::ios_base::fmtflags f(cout.flags());
			for (int i = 0; i < 16; i++) {
				cout << std::setfill('0') << std::setw(2) << hex << (unsigned int) imgbuf[i] << " ";
			}
			cout << endl;
			for (int i = 0; i < 8; i++) {
				cout << std::setfill('0') << std::setw(2) << hex << (unsigned int) labelbuf[i] << " ";
			}
			cout << endl;
			cout.flags(f);

			for (unsigned int i = 0; i < TEST_IMAGES; i++) {
				imgs.read(imgbuf, 28 * 28);
				labels.read(labelbuf, 1);

				for (int j = 0; j < 28 * 28; j++) {
					float dimg = (float) (unsigned char) imgbuf[j];
					dimg = dimg * 2.0f / 255.0f - 1.0f;
					unsigned int uimg = *(unsigned int *) &dimg;
					main_memory[base_addr + j] = uimg;
				}
				base_addr += 28 * 28;
				correctLabels[i] = labelbuf[0];
				main_memory[base_addr] = (unsigned int) (unsigned char) labelbuf[0];
				base_addr++;
			}
		}
		
		//Write to memory with simple interface
		bool Write(unsigned int addr, unsigned int data){
			wait(); //write costs 1 clock cycle
			//Write to the memory
			if(addr >= DRAM_BASE_ADDR && addr < DRAM_BASE_ADDR + DRAM_SIZE){
				//write to memory
				main_memory[addr - DRAM_BASE_ADDR] = data;
				return true;
			}else{
				//Error condition, just return false on error condition
				return false;
			}
		}
		
		//Read from memory with simple interface
		bool Read(unsigned int addr, unsigned int& data){
			wait(); wait(); //read costs 2 clock cycles
			//Read from the memory
			if(addr >= DRAM_BASE_ADDR && addr < DRAM_BASE_ADDR + DRAM_SIZE){
				//read to memory
				data = main_memory[addr - DRAM_BASE_ADDR];
				return true;
			}else{
				//Error condition, TODO: something
				return false;
			}
		}
		
};


