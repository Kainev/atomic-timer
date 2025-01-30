#pragma once

#include "NTPClient.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

class AtomicTimer {
public:
    AtomicTimer(
        const std::vector<std::string>& servers,
        double sync_rate = 3.0,
        double slew_rate = 0.5
    );

    ~AtomicTimer();

    void start();
    double stop();
    void reset();

    double real_time();
    double smooth_time();
    double local_time();

private:
    void _sync_loop();
    void _sync();

private:
    std::vector <NTPClient> _ntp_clients;
    double _sync_rate;
    double _slew_rate;

    std::thread _sync_thread;
    std::atomic<bool> _stop_sync;

    std::mutex _offset_mutex;
    double _real_offset;
    double _smoothed_offset;
    std::chrono::steady_clock::time_point _last_update;

    bool _running;
    std::chrono::steady_clock::time_point _start_time;
    double _start_offset; 
};