#include "AtomicTimer.h"
#include <cmath>
#include <iostream>

AtomicTimer::AtomicTimer(const std::vector<std::string>& servers, double sync_rate, double slew_rate):
    _sync_rate(sync_rate),
    _slew_rate(slew_rate),
    _stop_sync(false),
    _real_offset(0.0),
    _smoothed_offset(0.0),
    _start_offset(0.0),
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

    _start_time = std::chrono::steady_clock::now();
    _last_update = _start_time;

    _stop_sync = false;

    if(!_sync_thread.joinable())
    {
        _sync_thread = std::thread(&AtomicTimer::_sync_loop, this);
    }
    
}

void AtomicTimer::stop()
{
    if(!_running) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_sync_cv_mutex);
        _stop_sync = true;
    }

    _sync_cv.notify_one();

    _running = false;

    if(_sync_thread.joinable())
    {
        _sync_thread.join();
    }

    for(auto &client : _ntp_clients)
    {
        client.disconnect();
    }
}

void AtomicTimer::reset()
{
    std::lock_guard<std::mutex> lk(_offset_mutex);
    _start_time = std::chrono::steady_clock::now();
    _start_offset = _real_offset;
}

double AtomicTimer::real_time()
{
    if (!_running) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lk(_offset_mutex);

    double elapsed_locally = std::chrono::duration<double>(now - _start_time).count();
    return elapsed_locally + (_real_offset - _start_offset);
}

double AtomicTimer::smooth_time()
{
    if (!_running) {
        return 0.0;
    }

    auto now = std::chrono::steady_clock::now();
    double delta = std::chrono::duration<double>(now - _last_update).count();
    _last_update = now;

    std::lock_guard<std::mutex> lk(_offset_mutex);

    if (_slew_rate > 1e-9) {
        double diff = _real_offset - _smoothed_offset;
        double fraction = delta / _slew_rate;
        if (fraction > 1.0) {
            fraction = 1.0;
        }
        _smoothed_offset += diff * fraction;
    } else {
        _smoothed_offset = _real_offset;
    }

    double elapsed_locally = std::chrono::duration<double>(now - _start_time).count();
    return elapsed_locally + (_smoothed_offset - _start_offset);
}

double AtomicTimer::local_time() const
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - _start_time).count();
}

void AtomicTimer::_sync_loop()
{
    using namespace std::chrono;

    for (auto &client : _ntp_clients) {
        client.connect();
    }

    auto next_sync = steady_clock::now();

    while (true) {
        _sync();

        next_sync += duration_cast<steady_clock::duration>(duration<double>(_sync_rate));

        auto now = steady_clock::now();
        if (next_sync < now) {
            next_sync = now;
        }

        std::unique_lock<std::mutex> lock(_sync_cv_mutex);
        _sync_cv.wait_until(lock, next_sync, [this]() { return _stop_sync; });

        if(_stop_sync) {
            break;
        }
    }
}

void AtomicTimer::_sync()
{
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
