#include "AtomicTimer.h"
#include <cmath>
#include <iostream>

AtomicTimer::AtomicTimer(const std::vector<std::string>& servers, double sync_rate, double slew_rate):
    _sync_rate(sync_rate),
    _slew_rate(slew_rate),
    _stop_sync(false),
    _real_offset(0.0),
    _smoothed_offset(0.0),
    _running(false)
{
    for(auto &server : servers) {
        _ntp_clients.emplace_back(server);
    }

    _last_update = std::chrono::steady_clock::now();
}

AtomicTimer::~AtomicTimer()
{
    stop();
}

void AtomicTimer::start()
{
    if(_running) {
        return;
    }

    _running = true;

    for (auto &client : _ntp_clients)
    {
        client.connect();
    }

    {
        std::lock_guard<std::mutex> lk(_offset_mutex);
        _start_offset = _real_offset;
        _smoothed_offset = _real_offset;
    }

    _start_time = std::chrono::steady_clock::now();
    _last_update = _start_time;

    _stop_sync = false;

    if(!_sync_thread.joinable())
    {
        _sync_thread = std::thread(&AtomicTimer::_sync_loop, this);
    }
}

double AtomicTimer::stop()
{
    if(!_running) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();

    double end_offset;
    {
        std::lock_guard<std::mutex> lk(_offset_mutex);
        end_offset = _real_offset;
    }

    double elapsed_locally = std::chrono::duration<double>(now - _start_time).count();

    _running = false;
    _stop_sync = true;
    if(_sync_thread.joinable())
    {
        _sync_thread.join();
    }

    for(auto &client : _ntp_clients)
    {
        client.disconnect();
    }

    return elapsed_locally + (end_offset - _start_offset);;
}

void AtomicTimer::reset()
{
    _start_time = std::chrono::steady_clock::now();
}

double AtomicTimer::real_time()
{
    if(!_running) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed_locally = std::chrono::duration<double>(now - _start_time).count();

    double current_offset;
    {
        std::lock_guard<std::mutex> lk(_offset_mutex);
        current_offset = _real_offset;
    }

    return elapsed_locally + (current_offset - _start_offset);
}

double AtomicTimer::smooth_time()
{
    if(!_running) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();

    double delta = std::chrono::duration<double>(now - _last_update).count();
    _last_update = now;

    double target;
    {
        std::lock_guard<std::mutex> lk(_offset_mutex);
        target = _real_offset;
    }

    if(_slew_rate > 1e-9)
    {
        double diff = target - _smoothed_offset;
        double fraction = delta / _slew_rate;
        if(fraction > 1.0) fraction = 1.0;
        _smoothed_offset += diff * fraction;
    } else {
        _smoothed_offset = target;
    }

    double elapsed_locally = std::chrono::duration<double>(now - _start_time).count();
    return elapsed_locally + (_smoothed_offset - _start_offset);
}

double AtomicTimer::local_time()
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - _start_time).count();
}


void AtomicTimer::_sync_loop()
{
    while (!_stop_sync) {
        _sync();
        double remaining = _sync_rate;
        while(remaining > 0.0 && !_stop_sync) {
            constexpr double step = 0.1;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            remaining -= step;
        }
    }
}

void AtomicTimer::_sync()
{
    auto t0 = std::chrono::steady_clock::now();
    auto worker = [&](NTPClient* c, NTPResult* out) {
        *out = c->offset();
    };

    std::vector<std::thread> threads;
    std::vector<NTPResult> results(_ntp_clients.size());

    threads.reserve(_ntp_clients.size());
    for (size_t i = 0; i < _ntp_clients.size(); ++i) {
        threads.emplace_back(worker, &_ntp_clients[i], &results[i]);
    }

    for (auto &thread : threads) {
        thread.join();
    }

    auto t1 = std::chrono::steady_clock::now();

    double weights_sum = 0.0;
    double product_sum = 0.0;
    for (auto &result : results) {
        if(result.rtt >= 9999.0) {
            continue;
        }

        double weight = 1.0 / result.rtt;
        weights_sum += weight;
        product_sum += result.offset * weight;
    }

    if(weights_sum < 1e-9) {
        return;
    }

    double new_offset = product_sum / weights_sum;

    {
        std::lock_guard<std::mutex> lk(_offset_mutex);
        _real_offset = new_offset;
        if(!_start_offset) {
            _start_offset = new_offset;
            _smoothed_offset = new_offset;
        }
    }
}
