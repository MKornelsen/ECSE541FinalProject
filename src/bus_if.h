#pragma once 

#include <systemc.h>

#define OP_SINGLE_READ 0
#define OP_SINGLE_WRITE 1
#define OP_BURST_READ 2
#define OP_BURST_WRITE 3

class bus_master_if : virtual public sc_interface {
public:
    virtual void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) = 0;
    virtual void WaitForAcknowledge(unsigned int mst_id) = 0;
    virtual void ReadData(unsigned int &data) = 0;
    virtual void WriteData(unsigned int data) = 0;
};

class bus_minion_if : virtual public sc_interface {
public:
    virtual void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) = 0;
    virtual void Acknowledge() = 0;
    virtual void SendReadData(unsigned int data) = 0;
    virtual void ReceiveWriteData(unsigned int &data) = 0;
};
