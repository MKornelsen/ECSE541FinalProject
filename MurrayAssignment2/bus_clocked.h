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
    ACK_WAIT,
    SERVING_RQ
};

class bus_clocked : public sc_module, public bus_master_if, public bus_minion_if {
private:
    std::vector<std::deque<bus_request *>> request_queue;
    bus_request *cur_request;
    int rrcount;

    bus_state state;

    unsigned int bus_data;

    bool acknowledged;
    
    bool bus_ready, data_ready;

public:
    sc_in<sc_logic> clk;

    SC_HAS_PROCESS(bus_clocked);

    bus_clocked(sc_module_name name) : sc_module(name) {
        cur_request = NULL;
        state = IDLE;
        acknowledged = false;
        bus_ready = false;
        data_ready = false;
        
        SC_METHOD(bus_proc);
        sensitive << clk.pos();
    }

    void attach_master(unsigned int &id) {
        id = (unsigned int) request_queue.size();
        request_queue.push_back(std::deque<bus_request *>());
    }

    void bus_proc() {
        switch(state) {
        case IDLE:
            for (int i = 0; i < request_queue.size(); i++) {
                std::deque<bus_request *> &tmp = request_queue.at((rrcount + i) % request_queue.size());
                if (!tmp.empty()) {
                    cur_request = tmp.front();
                    tmp.pop_front();
                    state = SERVING_RQ;
                    break;
                }
            }
            rrcount++;
            break;
        case SERVING_RQ:
            // cout << cur_request->len << endl;
            if (cur_request->len <= 0) {
                state = IDLE;
                acknowledged = false;
                cur_request = NULL;
            }
            break;
        }
    }

    void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) {
        cout << "Request " << addr << " - " << sc_time_stamp() << endl;;
        wait(clk.posedge_event());
        wait(clk.posedge_event());
        bus_request *req = new bus_request(mst_id, addr, op, len);
        request_queue.at(mst_id).push_back(req);
    }

    void WaitForAcknowledge(unsigned int mst_id) {
        while (!acknowledged || cur_request == NULL || cur_request->mst_id != mst_id) {
            wait(clk.negedge_event());
        }
    }

    void ReadData(unsigned int &data) {
        bus_ready = true;
        while (!data_ready) {
            wait(clk.posedge_event());
        }

        cout << "ReadData - " << sc_time_stamp() << endl;
        data = bus_data;
        data_ready = false;
        
        if (cur_request->op == OP_SINGLE_READ || cur_request->op == OP_SINGLE_WRITE) {
            cur_request->len = 0;
        } else {
            cur_request->len--;
        }
        wait(clk.posedge_event());
        wait(clk.posedge_event());
    }

    void WriteData(unsigned int data) {
        while (!bus_ready) {
            wait(clk.posedge_event());
        }

        cout <<"WriteData - " << sc_time_stamp() << endl;
        bus_data = data;
        data_ready = true;
        bus_ready = false;
        wait(clk.posedge_event());
    }

    void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) {
        wait(clk.posedge_event());
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
        cout << "Acknowledge - " << sc_time_stamp() << endl;
        wait(clk.posedge_event());
        acknowledged = true;
    }

    void SendReadData(unsigned int data) {
        while (!bus_ready) {
            wait(clk.posedge_event());
        }

        cout << "SendReadData - " << sc_time_stamp() << endl;
        bus_data = data;
        data_ready = true;
        bus_ready = false;
        
        wait(clk.posedge_event());
        wait(clk.posedge_event());
    }

    void ReceiveWriteData(unsigned int &data) {
        bus_ready = true;
        while (!data_ready) {
            wait(clk.posedge_event());
        }
        cout << "ReceiveWriteData - " << sc_time_stamp() << endl;
        data = bus_data;
        data_ready = false;
        
        if (cur_request->op == OP_SINGLE_READ || cur_request->op == OP_SINGLE_WRITE) {
            cur_request->len = 0;
        } else {
            cur_request->len--;
            // cout << cur_request->len << endl;
        }
        wait(clk.posedge_event());
    }
};