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
#define BUS_MST_SW 1
#define BUS_MST_HW 2

#define OP_READ 5
#define OP_WRITE 6
#define OP_HW_MUL 7
#define OP_HW_STAT 8

//Defines for hardware block
#define STATUS_IP 9
#define STATUS_DONE 10


/** </INTERNAL DEFINES> **/


/** <EXTERNAL DEFINES> **/
#define DRAM_SIZE 1024
#define DRAM_START_ADDR 1024




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