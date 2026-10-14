#pragma once
#include "generators.h"
#include <deque>
#include <set>
#include <string>
#include <vector>

constexpr long long MinuteMs = 60000;
constexpr long long HourMs = 60 * MinuteMs;
constexpr long long DayMs = 24 * HourMs;
constexpr int SimDays = 28;

struct DistRange { // распределение
    bool normal = false;
    int min_ms = 0;
    int max_ms = 1;
    double mean_ms = 0;
    double sigma_ms = 1;
};

struct BankParams {
    int clerks = 3; // N 2-7
    int limit = 15; // K 10-25
    DistRange arrival;
    DistRange service;
    int profit_min = 50; // в рублях
    int profit_max = 3000;
    long long seed = 1;
};

struct ClerkView {
    enum Phase { Free, Serving, Lunch } phase = Free;
    int client = 0;
    long long until = 0;
};

struct BankSnapshot {
    long long time = 0;
    bool open = false;
    bool done = false;
    int issued_today = 0;
    int board_client = 0;
    int board_window = 0;
    int queue_limit = 0;
    bool queue_ready = false;
    std::vector<int> queue;
    std::vector<ClerkView> clerks;
    int served = 0;
    int lost = 0;
    int lost_full = 0;
    int lost_close = 0;
    int queue_min = 0;
    int queue_max = 0;
    double queue_avg = 0;
    double wait_avg_min = 0;
    double occupancy = 0;
    long long income = 0;
    long long salary = 0;
    long long profit = 0;
    std::vector<std::string> log;
};

class Bank {
public:
    Bank(const BankParams &params);

    long long now() const { return cur; }
    long long end_time() const { return (long long) SimDays * DayMs; }
    bool finished() const { return cur >= end_time(); }
    static std::string clock_text(long long t);

private:
    enum Kind {
        Close = 0,
        Open = 1,
        ServiceEnd = 2,
        LunchEnd = 3,
        LunchStart = 4,
        Arrival = 5,
        Hour = 6
    };

    struct Timed {
        long long time;
        int prio;
        long long seq;
        Kind kind;
        int clerk;
        bool operator<(const Timed &o) const {
            if (time != o.time) return time < o.time;
            if (prio != o.prio) return prio < o.prio;
            return seq < o.seq;
        }
    };

    struct Waiting {
        int number;
        long long arrival;
    };

    struct Clerk {
        ClerkView::Phase phase = ClerkView::Free;
        int client = 0;
        long long arrived = 0;
        long long started = 0;
        int service = 0;
        int profit = 0;
        long long until = 0;
        bool lunch_done = false;
        long long lunch_nominal = 0;
    };

    struct Plan {
        bool active = false;
        long long at = 0;
        long long anchor = 0;
        double m = 1;
        double work = 0;
        Timed key;
    };

    BankParams params;
    Generators gen;
    std::set<Timed> events;
    long long seq = 0;
    long long cur = 0;
    int day_index = -1;

    bool accepting = false;
    std::deque<Waiting> queue;
    int L = 0;
    std::vector<Clerk> clerks;
    Plan plan;

    int next_number = 0;
    int board_client = 0;
    int board_window = 0;

    int served = 0;
    int lost = 0;
    int lost_full = 0;
    int lost_close = 0;
    bool queue_ready = false;
    int queue_min = 0;
    int queue_max = 0;
    long long sum_queue = 0;
    long long sum_busy = 0;
    long long open_ms = 0;
    long long stats_anchor = 0;
    long long sum_wait = 0;
    long long wait_n = 0;
    long long income = 0;
    long long salary = 0;
    std::deque<std::string> logs;

    void schedule(long long time, Kind kind, int clerk);
    void schedule_next_open(int day);
    void accrue(long long t);
    void note_queue();
    void log(const std::string &text);
    double speed() const;
    void arm_new_interval();
    void retarget();
    void cancel_arrival();
    int find_free() const;
    void start_service(int clerk, int number, long long arrived);
    void try_assign(int clerk);

    void on_arrival();
    void on_service_end(int clerk);
};
