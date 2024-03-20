#pragma once

#include <cstdint>

class timer_state
{
private:
  bool timer_running_state {};
  uint64_t timer_ms {};
  uint64_t re_trans_count {};
  uint64_t RTO_ms{};
public:
    timer_state() {};
    void state_reset(uint64_t initial_RTO_ms){timer_ms=0; RTO_ms=initial_RTO_ms;};
    void state_off(){timer_running_state = false;};
    void run_on(){timer_running_state=true;};
    bool is_running() const{return timer_running_state;};
    bool check_out_of_date(uint64_t add_ms) {if(timer_running_state) timer_ms+=add_ms; return timer_running_state && timer_ms>=RTO_ms;};
    void add_count() {++re_trans_count;}
    void clear_count () {re_trans_count=0;};
    uint64_t get_current_RTO () const {return RTO_ms;};
    uint64_t peek_count() const {return re_trans_count;}
};


