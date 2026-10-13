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
    Bank();
};
