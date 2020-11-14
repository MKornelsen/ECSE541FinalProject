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

class bus_unclocked : public sc_module, public bus_master_if, public bus_minion_if {
private:
    std::vector<std::deque<bus_request *>> request_queue;
    int rrcount;
    bus_request *cur_request;

    bus_state state;

    unsigned int bus_data;

    // bool acknowledged;
    
    // bool bus_ready, data_ready;

    bool acknowledged, bus_ready, data_ready;
    sc_event ack_event, bus_ready_event, data_ready_event, rq_complete_event;

    sc_event notify_listeners_event;
    sc_event ready_for_next_event;

public:
    SC_HAS_PROCESS(bus_unclocked);

    bus_unclocked(sc_module_name name) : sc_module(name) {
        cur_request = NULL;
        state = IDLE;
        acknowledged = false;
        bus_ready = false;
        data_ready = false;

        SC_THREAD(bus_proc);
    }

    void attach_master(unsigned int &id) {
        id = (unsigned int) request_queue.size();
        request_queue.push_back(std::deque<bus_request *>());
    }

    void bus_proc() {
        while (true) {
            switch(state) {
            case IDLE:
                // cout << "IDLE" << endl;
                wait(SC_ZERO_TIME);
                for (int i = 0; i < request_queue.size(); i++) {
                    std::deque<bus_request *> &tmp = request_queue.at((rrcount + i) % request_queue.size());
                    if (!tmp.empty()) {
                        cur_request = tmp.front();
                        tmp.pop_front();
                        state = SERVING_RQ;
                        break;
                    }
                }
                // if (cur_request == NULL) {
                //     cout << "cur_request null" << endl;
                // } else {
                //     cout << "cur_request " << cur_request->addr << endl;
                // }
                notify_listeners_event.notify();
                rrcount++;
                break;
            case SERVING_RQ:
                // cout << cur_request->len << endl;
                wait(rq_complete_event);
                // cout << "rq_complete_event " << cur_request->addr << " " << cur_request->len << endl;
                if (cur_request->len <= 0) {
                    // cout << "rq done transitioning state" << endl;
                    state = IDLE;
                    acknowledged = false;
                    delete cur_request;
                    cur_request = NULL;
                }
                ready_for_next_event.notify();
                break;
            }
        }
    }

    void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) {     
        // cout << "Request: " << mst_id << " " << addr << " " << op << " " << endl;
        bus_request *req = new bus_request(mst_id, addr, op, len);
        request_queue.at(mst_id).push_back(req);
    }

    void WaitForAcknowledge(unsigned int mst_id) {
        // cout << "WaitForAcknowledge" << endl;
        while (!acknowledged || cur_request == NULL || cur_request->mst_id != mst_id) {
            wait(ack_event);
        }
        // cout << "ACK " << mst_id << " " << cur_request->addr << endl;
    }

    void ReadData(unsigned int &data) {
        bus_ready = true;
        bus_ready_event.notify();

        if (!data_ready) {
            wait(data_ready_event);
        }
        
        data = bus_data;
        data_ready = false;
        
        if (cur_request->op == OP_SINGLE_READ || cur_request->op == OP_SINGLE_WRITE) {
            cur_request->len = 0;
        } else {
            cur_request->len--;
        }
        // cout << "ReadData len = " << cur_request->len << endl;

        rq_complete_event.notify();
        wait(ready_for_next_event);
    }

    void WriteData(unsigned int data) {
        if (!bus_ready) {
            wait(bus_ready_event);
        }
        // cout << "SendWriteData " << data << endl;
        bus_data = data;
        data_ready = true;
        bus_ready = false;

        data_ready_event.notify();
        
        wait(ready_for_next_event);
    }

    void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) {
        wait(notify_listeners_event);
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
        acknowledged = true;
        ack_event.notify();
    }

    void SendReadData(unsigned int data) {
        if (!bus_ready) {
            wait(bus_ready_event);
        }

        bus_data = data;
        data_ready = true;
        bus_ready = false;
        data_ready_event.notify();

        wait(ready_for_next_event);
        // cout << "SendReadData done" << endl;
    }

    void ReceiveWriteData(unsigned int &data) {
        bus_ready = true;
        bus_ready_event.notify();

        if (!data_ready) {
            // cout << "!data_ready" << endl;
            wait(data_ready_event);
        }
        // cout << "ReceiveWriteData " << bus_data << endl;
        data = bus_data;
        data_ready = false;
        
        if (cur_request->op == OP_SINGLE_READ || cur_request->op == OP_SINGLE_WRITE) {
            cur_request->len = 0;
        } else {
            cur_request->len--;
            // cout << cur_request->len << endl;
        }
        
        rq_complete_event.notify();
        wait(ready_for_next_event);
    }
};