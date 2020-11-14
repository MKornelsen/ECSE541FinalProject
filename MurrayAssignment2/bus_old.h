#pragma once

#include <systemc.h>
#include <queue>

#include "bus_if.h"

struct bus_request {
    unsigned int mst_id;
    unsigned int addr;
    unsigned int op;
    unsigned int len;

    bus_request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len)
        : mst_id(mst_id)
        , addr(addr)
        , op(op)
        , len(len) { }
};

enum bus_state {
    IDLE,
    SERVING_RQ
};

class bus : public sc_module, public bus_master_if, public bus_minion_if {
private:
    std::vector<std::deque<bus_request>> requests;
    bus_request *cur_request;
    int rrcount;

    sc_event req_updated;
    sc_event ack_event;

    sc_event master_action;
    sc_event minion_action;

    sc_event data_ready;

    sc_event rq_complete;

    bus_state state;

    unsigned int databuf;
    
public:
    SC_HAS_PROCESS(bus);

    bus(sc_module_name name) : sc_module(name) {
        cur_request = NULL;
        rrcount = 0;
        SC_THREAD(bus_proc);
        state = IDLE;
        sensitive << ack_event << minion_action << master_action;
    }

    void attach_master(unsigned int &id) {
        id = (unsigned int) requests.size();
        requests.push_back(std::deque<bus_request>());
    }

    void bus_proc() {
        while (true) {
            switch (state) {
            case IDLE:
                wait(SC_ZERO_TIME);
                cout << "Picking cur_request" << endl;

                if (requests.at(rrcount % requests.size()).size() > 0) {
                    cur_request = &requests.at(rrcount % requests.size()).front();
                    requests.at(rrcount % requests.size()).pop_front();
                    req_updated.notify();
                    state = SERVING_RQ;
                }
                rrcount++;
                
                break;
            case SERVING_RQ:
                cout << "Waiting for rq_complete" << endl;
                wait(rq_complete);
                cout << "rq_complete received" << endl;
                cur_request->len--;
                if (cur_request->len <= 0 || cur_request->op == OP_SINGLE_READ || cur_request->op == OP_SINGLE_WRITE) {
                    state = IDLE;
                    cur_request = NULL;
                }
                break;
            }   
        }
    }

    void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) {
        cout << "Request " << addr << endl;
        bus_request req(mst_id, addr, op, len);
        requests.at(mst_id).push_back(req);
    }

    void WaitForAcknowledge(unsigned int mst_id) {
        while (true) {
            wait(ack_event);
            if (cur_request->mst_id == mst_id) {
                cout << "WaitForAcknowledge done" << endl;
                return;
            }
        }
    }

    void ReadData(unsigned int &data) {
        if (cur_request->op == OP_BURST_WRITE || cur_request->op == OP_SINGLE_WRITE) {
            return;
        }
        data_ready.notify();
        wait(minion_action);
        data = databuf;
        cout << "ReadData " << data << endl;
        rq_complete.notify();
    }

    void WriteData(unsigned int data) {
        if (cur_request->op == OP_BURST_READ || cur_request->op == OP_SINGLE_READ) {
            return;
        }
        cout << "WriteData " << data << endl;
        databuf = data;
        data_ready.notify();
        wait(minion_action);
        rq_complete.notify();
    }

    void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) {
        wait(req_updated);
        cout << "Sending listen data" << endl;
        if (cur_request == NULL) {
            req_addr = -1;
            req_op = -1;
            req_len = -1;
            return;
        }
        req_addr = cur_request->addr;
        req_op = cur_request->op;
        req_len = cur_request->len;
    }

    void Acknowledge() {
        ack_event.notify();
    }

    void SendReadData(unsigned int data) {
        if (cur_request->op == OP_BURST_WRITE || cur_request->op == OP_SINGLE_WRITE) {
            return;
        }
        wait(data_ready);
        cout << "SendReadData " << data << endl;
        databuf = data;
        minion_action.notify();
        wait(rq_complete);
    }

    void ReceiveWriteData(unsigned int &data) {
        if (cur_request->op == OP_BURST_READ || cur_request->op == OP_SINGLE_READ) {
            return;
        }
        wait(data_ready);
        cout << "ReceiveWriteData " << databuf << endl;
        data = databuf;
        minion_action.notify();
        wait(rq_complete);
    }
};
