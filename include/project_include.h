/**
Include file for Assignment 1
**/

//This just checks if the file has already been included
#ifndef PROJECT_INCLUDE
#define PROJECT_INCLUDE

/** <INTERNAL DEFINES> **/

//Defines for the memory block
#define SIZE_ARRAY 36
#define ARRAY_SIZE 36
#define ADDR_A 0
#define ADDR_B 36
#define ADDR_C 72
#define MEM_SIZE 108
#define ADDR_HW_MASTER 108
#define ADDR_HW_MINION 109

//Defines for the bus block
#define BUS_MST_SW 0
#define BUS_MST_HW 1

#define OP_READ 5
#define OP_WRITE 6
#define OP_HW_MUL 7
#define OP_HW_STAT 8

//Defines for hardware block
#define STATUS_IP 9
#define STATUS_DONE 10


/** </INTERNAL DEFINES> **/
#define TEST_IMAGES 2500

/** <EXTERNAL DEFINES> **/
#define DRAM_BASE_ADDR 1024
#define DRAM_SIZE 0x08000000

#define NUM_LAYERS 6

#define LAYER_SIZES {784, 2500, 2000, 1500, 1000, 500, 10};

#define NUM_ACCELERATORS 4

#define EIE_CC_BASE_ADDR 0
#define EIE_CC_ADDR_SIZE 0x100
#define EIE_CC_ADDR_OP 0
#define EIE_CC_ADDR_DATA 1
#define EIE_CC_ADDR_LEN 2
#define EIE_CC_ADDR_ROWS 3
#define EIE_CC_ADDR_ROWLEN 4
#define EIE_CC_ADDR_LAYER 5
#define EIE_CC_ADDR_OUTREADY 6
#define EIE_CC_ADDR_OP_COMPLETE 7
#define EIE_CC_ADDR_PREDICTED_LABEL 8

#define EIE_CC_OP_WRITE_WEIGHT 1
#define EIE_CC_OP_WRITE_INPUT 2
#define EIE_CC_OP_READ_OUTPUT 3

/** </EXTERNAL DEFINES> **/







/** <Interface and Class definitions> **/

//Definition of the virtual class, overwrite the implementations
class simple_mem_if : virtual public sc_interface
{
  public:
    virtual bool Write(unsigned int addr, unsigned int data) = 0;
    virtual bool Read(unsigned int addr, unsigned int& data) = 0;
};

// Bus Master Interface
class bus_master_if : virtual public sc_interface
{
  public:
    virtual void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) = 0;
    virtual bool WaitForAcknowledge(unsigned int mst_id) = 0;
    virtual void ReadData(unsigned int &data) = 0;
    virtual void WriteData(unsigned int data) = 0;
};


// Bus Minion Interface 
class bus_minion_if : virtual public sc_interface
{
  public:
    virtual void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) = 0;
    virtual void Acknowledge() = 0; 
    virtual void SendReadData(unsigned int data) = 0;
    virtual void ReceiveWriteData(unsigned int &data) = 0;
};

/** </Interface and Class definitions> **/



/** <STRUCT DEFINITIONS> **/

//This is the request data structure. I will use it to keep track of requests.
struct request_t {
	unsigned int mst_id;
	unsigned int addr;
	unsigned int op;
	unsigned int len;
	bool acknowledged; //has this request been acknowledged by the minion?
	request_t * next; //pointer to next request. 
};

/** <\STRUCT DEFINITIONS> **/


#endif