import time
from atomic_timer import AtomicTimer


def main():
    servers = [
        "0.au.pool.ntp.org",
        "1.au.pool.ntp.org",
        "2.au.pool.ntp.org",
    ]
    
    timer = AtomicTimer(servers, sync_rate=1.0, slew_rate=0.3)
    timer.start()
    
    try:
        for i in range(10000):
            real_time = timer.real_time()
            smooth_time = timer.smooth_time()
            local_time = timer.local_time()
            
            print(f"\r[{i}] real_time={real_time:.4f}s, smooth_time={smooth_time:.4f}s, local_time={local_time:.4f}s", end='')
            
            time.sleep(0.01) 
    except KeyboardInterrupt:
        pass
    finally:
        elapsed = timer.stop()  
        print(f"\nFinal time: {elapsed:.4f} s")


if __name__ == "__main__":
    main()
