#pragma once
#include <chrono>

using namespace std::chrono_literals;

template <class DT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock>
class Timer
{
    using timep_t = decltype(ClockT::now());
    
    timep_t _start = ClockT::now();
    timep_t _end = {};

public:
    void tick() { 
        _end = timep_t{};
        _start = ClockT::now(); 
    }
    
    void tock() {
        _end = ClockT::now(); 
    }
    
    template <class duration_t = DT>
    auto duration() const {
        return std::chrono::duration_cast<duration_t>(_end - _start); 
    }
};