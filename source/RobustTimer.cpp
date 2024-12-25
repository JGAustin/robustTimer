#include <iostream>
#include <functional>
#include <stdexcept>
#include <signal.h>
#include <time.h>
#include <atomic>
#include <semaphore.h>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>

#include <RobustTimer.hpp>

std::map<int, RobustTimer*> RobustTimer::signal_map_;
std::mutex RobustTimer::signal_mutex_;

RobustTimer::RobustTimer(long timeout_ns, std::function<void()> callback)
    : timeout_ns_(timeout_ns), callback_(std::move(callback)), is_running_(false) {
    if (!callback_) {
        throw std::invalid_argument("Callback function cannot be null.");
    }

    // Initialize the semaphore
    if (sem_init(&semaphore_, 0, 0) != 0) {
        throw std::runtime_error("Failed to initialize semaphore.");
    }

    // Allocate a unique signal number for this instance
    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = RobustTimer::signalHandler;

    {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        for (int signo = SIGRTMIN; signo <= SIGRTMAX; ++signo) {
            if (signal_map_.find(signo) == signal_map_.end()) {
                sigaction(signo, &sa, nullptr);
                signal_number_ = signo;
                signal_map_[signo] = this;
                break;
            }
        }
    }

    if (signal_number_ == -1) {
        throw std::runtime_error("No available signal numbers for the timer.");
    }

    // Initialize the timer
    struct sigevent sev = {};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signal_number_;
    sev.sigev_value.sival_ptr = this;

    if (timer_create(CLOCK_REALTIME, &sev, &timer_id_) == -1) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        signal_map_.erase(signal_number_);
        throw std::runtime_error("Failed to create timer.");
    }

    // Start the worker thread
    worker_thread_ = std::thread(&RobustTimer::workerThread, this);
}

RobustTimer::~RobustTimer() {
    stop();
    timer_delete(timer_id_);
    sem_post(&semaphore_); // Unblock the worker thread
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    sem_destroy(&semaphore_);

    std::lock_guard<std::mutex> lock(signal_mutex_);
    signal_map_.erase(signal_number_);
}

void RobustTimer::start() {
    if (is_running_.exchange(true)) {
        return; // Timer already running
    }

    armTimer(timeout_ns_);
}

void RobustTimer::stop() {
    if (!is_running_.exchange(false)) {
        return; // Timer already stopped
    }

    disarmTimer();
}

void RobustTimer::changeTimeout(long new_timeout_ns) {
    timeout_ns_ = new_timeout_ns;
    if (is_running_) {
        armTimer(timeout_ns_);
    }
}

static void RobustTimer::signalHandler(int signo, siginfo_t* info, void* context) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    auto it = signal_map_.find(signo);
    if (it != signal_map_.end() && it->second) {
        sem_post(&it->second->semaphore_);
    }
}

void RobustTimer::workerThread() {
    while (true) {
        sem_wait(&semaphore_);

        if (!is_running_) {
            break; // Exit thread if timer is stopped
        }

        if (callback_) {
            callback_();
        }
    }
}

void RobustTimer::armTimer(long timeout_ns) {
    struct itimerspec its = {};
    its.it_value.tv_sec = timeout_ns / 1000000000;
    its.it_value.tv_nsec = timeout_ns % 1000000000;
    its.it_interval.tv_sec = 0; // One-shot timer
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timer_id_, 0, &its, nullptr) == -1) {
        throw std::runtime_error("Failed to arm the timer.");
    }
}

void RobustTimer::disarmTimer() {
    struct itimerspec its = {};
    if (timer_settime(timer_id_, 0, &its, nullptr) == -1) {
        throw std::runtime_error("Failed to disarm the timer.");
    }
}