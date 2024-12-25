#include <functional>
#include <stdexcept>
#include <time.h>
#include <atomic>
#include <map>
#include <mutex>

class RobustTimer {
public:
    RobustTimer(long timeout_ns, std::function<void()> callback);
    ~RobustTimer();

    void start();
    void stop();
    void changeTimeout(long new_timeout_ns);

private:
    timer_t timer_id_;
    long timeout_ns_;
    std::function<void()> callback_;
    std::atomic<bool> is_running_;
    sem_t semaphore_;
    std::thread worker_thread_;
    int signal_number_ = -1;

    static std::map<int, RobustTimer*> signal_map_;
    static std::mutex signal_mutex_;

    static void signalHandler(int signo, siginfo_t* info, void* context);
    void workerThread();
    void armTimer(long timeout_ns);
    void disarmTimer();
};

std::map<int, RobustTimer*> RobustTimer::signal_map_;
std::mutex RobustTimer::signal_mutex_;