#include "AtomicTimer.h" 

#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <csignal>   
#include <iomanip> 


std::atomic<bool> g_running(true);

void sigint_handler(int signal) {
    if (signal == SIGINT) {
        g_running = false;
    }
}


int main()
{
    std::signal(SIGINT, sigint_handler);

    std::vector<std::string> servers = {
        "0.au.pool.ntp.org",
        "1.au.pool.ntp.org",
        "2.au.pool.ntp.org",
    };

    double sync_rate = 1.0;  // seconds
    double slew_rate = 0.3;  // seconds
    AtomicTimer timer(servers, sync_rate, slew_rate);


    timer.start();

    int i = 0;
    while (g_running && i < 10000) {
        double real_time = timer.real_time();
        double smooth_time = timer.smooth_time();
        double local_time = timer.local_time();

        std::cout << "\r[" << i << "] "
                  << "real_time=" << std::fixed << std::setprecision(4) << real_time << "s, "
                  << "smooth_time=" << smooth_time << "s, "
                  << "local_time=" << local_time << "s"
                  << std::flush;

        ++i;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    double elapsed = timer.stop();
    std::cout << "\nFinal time: " << std::fixed << std::setprecision(4) << elapsed << " seconds\n";

    return 0;
}
