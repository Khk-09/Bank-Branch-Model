#include "bank.h"
#include <algorithm>
#include <cmath>

constexpr long long LunchMs = 30 * MinuteMs;
constexpr long long OpenWeekday = 9 * HourMs;
constexpr long long CloseWeekday = 17 * HourMs;
constexpr long long OpenSaturday = 10 * HourMs;
constexpr long long CloseSaturday = 16 * HourMs;
constexpr int SalaryRub = 2000;
constexpr int LogLimit = 400;

int weekday_of_day(int day) {
    return day % 7;
}

bool is_saturday(int day) {
    return weekday_of_day(day) == 5;
}

std::string minutes_text(long long ms) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", ms / double(MinuteMs));
    return buf;
}

Bank::Bank(const BankParams &params) : params(params), gen(params.seed) {
    if (this->params.clerks < 2) this->params.clerks = 2;
    if (this->params.clerks > 7) this->params.clerks = 7;
    if (this->params.limit < 10) this->params.limit = 10;
    if (this->params.limit > 25) this->params.limit = 25;
    clerks.assign(this->params.clerks, Clerk());
    schedule_next_open(0);
}

void Bank::schedule(long long time, Kind kind, int clerk) {
    Timed ev;
    ev.time = time;
    ev.prio = int(kind);
    ev.seq = ++seq;
    ev.kind = kind;
    ev.clerk = clerk;
    events.insert(ev);
}

void Bank::schedule_next_open(int day) {
    while (day < SimDays && weekday_of_day(day) == 6) day++;
    if (day >= SimDays) return;
    long long open_off = is_saturday(day) ? OpenSaturday : OpenWeekday;
    schedule((long long) day * DayMs + open_off, Open, -1);
}

void Bank::accrue(long long t) {
    if (t < stats_anchor) return;
    if (accepting && t > stats_anchor) {
        long long dt = t - stats_anchor;
        sum_queue += (long long) L * dt;
        int busy = 0;
        for (const Clerk &c : clerks) {
            if (c.phase == ClerkView::Serving) busy++;
        }
        sum_busy += (long long) busy * dt;
        open_ms += dt;
    }
    stats_anchor = t;
}

void Bank::note_queue() {
    if (!queue_ready) {
        queue_min = queue_max = L;
        queue_ready = true;
        return;
    }
    queue_min = std::min(queue_min, L);
    queue_max = std::max(queue_max, L);
}

void Bank::log(const std::string &text) {
    logs.push_back(clock_text(cur) + "  " + text);
    while (logs.size() > LogLimit) logs.pop_front();
}

std::string Bank::clock_text(long long t) {
    static const char *names[] = {"Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"};
    if (t < 0) t = 0;
    int day = t / DayMs;
    int wd = day % 7;
    int week = day / 7 + 1;
    long long tod = t % DayMs;
    int hh = tod / HourMs;
    int mm = tod % HourMs / MinuteMs;
    int ss = tod % MinuteMs / 1000;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Нед %d %s %02d:%02d:%02d", week, names[wd], hh, mm, ss);
    return buf;
}

double Bank::speed() const {
    int wd = weekday_of_day(std::max(day_index, 0));
    int H = is_saturday(day_index) ? 6 : 8;
    long long open_off = is_saturday(day_index) ? OpenSaturday : OpenWeekday;
    long long tod = cur % DayMs;
    int h = (tod - open_off) / HourMs;
    if (h < 0) h = 0;
    if (h >= H) h = H - 1;
    double m_week = (46.0 / 156.0) * (wd + 1);
    double m_hour = 2.0 * (h + 1) / (H + 1);
    double m_queue = 1.0;
    if (L > 7) {
        m_queue = double(params.limit - L + 1) / (params.limit - 6);
    }
    return m_week * m_hour * m_queue;
}

void Bank::arm_new_interval() {
    if (!accepting) return;
    if (plan.active) {
        events.erase(plan.key);
        plan.active = false;
    }
    int gap = std::max(1, gen.sample(params.arrival.min_ms, params.arrival.max_ms,
                                     params.arrival.normal, params.arrival.mean_ms, params.arrival.sigma_ms));
    plan.work = gap;
    plan.anchor = cur;
    plan.m = speed();
    long long dt = 0;
    if (plan.m > 0) dt = std::ceil(plan.work / plan.m - 1e-12);
    dt = std::max(0ll, dt);
    plan.at = cur + dt;
    plan.key.time = plan.at;
    plan.key.prio = int(Arrival);
    plan.key.seq = ++seq;
    plan.key.kind = Arrival;
    plan.key.clerk = -1;
    events.insert(plan.key);
    plan.active = true;
}

void Bank::retarget() {
    if (!plan.active || !accepting) return;
    double elapsed = std::max(0ll, cur - plan.anchor);
    plan.work -= elapsed * plan.m;
    if (plan.work < 1e-6) plan.work = 0;
    plan.anchor = cur;
    plan.m = speed();
    events.erase(plan.key);
    long long dt = 0;
    if (plan.work > 0 && plan.m > 0) dt = std::ceil(plan.work / plan.m - 1e-12);
    dt = std::max(0ll, dt);
    plan.at = cur + dt;
    plan.key.time = plan.at;
    plan.key.prio = int(Arrival);
    plan.key.seq = ++seq;
    plan.key.kind = Arrival;
    plan.key.clerk = -1;
    events.insert(plan.key);
}

void Bank::cancel_arrival() {
    if (!plan.active) return;
    events.erase(plan.key);
    plan.active = false;
    plan.work = 0;
}

int Bank::find_free() const {
    for (int i = 0; i < clerks.size(); i++) {
        if (clerks[i].phase == ClerkView::Free) return i;
    }
    return -1;
}

void Bank::start_service(int clerk, int number, long long arrived) {
    Clerk &c = clerks[clerk];
    int dur = std::max(0, gen.sample(params.service.min_ms, params.service.max_ms,
                         params.service.normal, params.service.mean_ms, params.service.sigma_ms));
    c.phase = ClerkView::Serving;
    c.client = number;
    c.arrived = arrived;
    c.started = cur;
    c.service = dur;
    c.profit = gen.randomInt(params.profit_min, params.profit_max);
    c.until = cur + dur;
    board_client = number;
    board_window = clerk + 1;
    schedule(c.until, ServiceEnd, clerk);
}

void Bank::begin_lunch(int clerk) {
    Clerk &c = clerks[clerk];
    c.phase = ClerkView::Lunch;
    c.lunch_done = true;
    c.client = 0;
    c.until = cur + LunchMs;
    schedule(c.until, LunchEnd, clerk);
    log("Окно " + std::to_string(clerk + 1) + " ушло на обед");
}

void Bank::try_assign(int clerk) {
    Clerk &c = clerks[clerk];
    if (!accepting) {
        c.phase = ClerkView::Free;
        c.client = 0;
        c.until = 0;
        return;
    }
    if (!c.lunch_done && cur >= c.lunch_nominal) {
        begin_lunch(clerk);
        return;
    }
    if (!queue.empty()) {
        Waiting w = queue.front();
        queue.pop_front();
        L--;
        note_queue();
        start_service(clerk, w.number, w.arrival);
        log("Окно " + std::to_string(clerk + 1) + " взяло клиента " + std::to_string(w.number));
        retarget();
        return;
    }
    c.phase = ClerkView::Free;
    c.client = 0;
    c.until = 0;
}

void Bank::on_open() {
    accrue(cur);
    accepting = true;
    day_index = int(cur / DayMs);
    next_number = 0;
    board_client = 0;
    board_window = 0;
    queue.clear();
    L = 0;
    note_queue();

    long long day0 = (long long)day_index * DayMs;
    long long open_off = is_saturday(day_index) ? OpenSaturday : OpenWeekday;
    long long close_off = is_saturday(day_index) ? CloseSaturday : CloseWeekday;
    int H = is_saturday(day_index) ? 6 : 8;
    schedule(day0 + close_off, Close, -1);
    for (int i = 1; i < H; i++) schedule(day0 + open_off + i * HourMs, Hour, -1);

    long long len = close_off - open_off;
    long long band = (long long)clerks.size() * LunchMs;
    long long t0 = open_off + (len - band) / 2;
    for (int i = 0; i < clerks.size(); i++) {
        clerks[i] = Clerk();
        clerks[i].lunch_nominal = day0 + t0 + i * LunchMs;
        schedule(clerks[i].lunch_nominal, LunchStart, i);
    }
    log(is_saturday(day_index) ? "Филиал открыт (короткий день)" : "Филиал открыт");
    arm_new_interval();
}

void Bank::on_close() {
    accrue(cur);
    accepting = false;
    cancel_arrival();
    int sent = 0;
    while (!queue.empty()) {
        Waiting w = queue.front();
        queue.pop_front();
        lost++;
        lost_close++;
        sum_wait += cur - w.arrival;
        wait_n++;
        sent++;
    }
    L = 0;
    salary += (long long)clerks.size() * SalaryRub;
    if (sent > 0) log("Закрытие: распущено " + std::to_string(sent));
    else log("Филиал закрыт");
    schedule_next_open(day_index + 1);
}

void Bank::on_arrival() {
    accrue(cur);
    if (!accepting) return;
    if (L >= params.limit) {
        lost++;
        lost_full++;
        log("Отказ: в очереди уже " + std::to_string(L));
    } else {
        int number = ++next_number;
        int seat = find_free();
        if (seat >= 0) {
            start_service(seat, number, cur);
            log("Клиент " + std::to_string(number) + " сразу к окну " + std::to_string(seat + 1));
        } else {
            queue.push_back(Waiting{number, cur});
            L++;
            note_queue();
            log("Клиент " + std::to_string(number) + " в очереди (" + std::to_string(L) + ")");
        }
    }
    arm_new_interval();
}

void Bank::on_service_end(int clerk) {
    accrue(cur);
    Clerk &c = clerks[clerk];
    if (c.phase == ClerkView::Serving) {
        served++;
        income += c.profit;
        sum_wait += c.started - c.arrived;
        wait_n++;
        log("Окно " + std::to_string(clerk + 1) + ": клиент " + std::to_string(c.client)
            + " обслужен, ожидание " + minutes_text(c.started - c.arrived) + " мин");
    }
    c.phase = ClerkView::Free;
    c.client = 0;
    try_assign(clerk);
}

void Bank::on_lunch_start(int clerk) {
    accrue(cur);
    if (clerk < 0 || clerk >= clerks.size()) return;
    if (clerks[clerk].lunch_done) return;
    if (clerks[clerk].phase == ClerkView::Free) begin_lunch(clerk);
}

void Bank::on_lunch_end(int clerk) {
    accrue(cur);
    if (clerk < 0 || clerk >= clerks.size()) return;
    clerks[clerk].phase = ClerkView::Free;
    clerks[clerk].client = 0;
    log("Окно " + std::to_string(clerk + 1) + " вернулось с обеда");
    try_assign(clerk);
}

void Bank::on_hour() {
    accrue(cur);
    retarget();
}

void Bank::advance_to(long long target) {
    if (target > end_time()) target = end_time();
    int guard = 0;
    while (!events.empty() && events.begin()->time <= target) {
        if (++guard > 5000000) return;
        Timed ev = *events.begin();
        events.erase(events.begin());
        cur = ev.time;
        if (ev.kind == Arrival) plan.active = false;
        switch (ev.kind) {
        case Open: on_open(); break;
        case Close: on_close(); break;
        case Arrival: on_arrival(); break;
        case ServiceEnd: on_service_end(ev.clerk); break;
        case LunchStart: on_lunch_start(ev.clerk); break;
        case LunchEnd: on_lunch_end(ev.clerk); break;
        case Hour: on_hour(); break;
        }
    }
    if (cur < target) {
        accrue(target);
        cur = target;
    }
}

void Bank::advance_step(long long step) {
    if (step < 1) step = 1;
    if (!accepting && events.empty()) {
        advance_to(end_time());
        return;
    }
    if (!accepting && !events.empty()) {
        advance_to(events.begin()->time);
        return;
    }
    advance_to(cur + step);
}

BankSnapshot Bank::snapshot() const {
    BankSnapshot s;
    s.time = cur;
    s.open = accepting;
    s.done = finished();
    s.issued_today = next_number;
    s.board_client = board_client;
    s.board_window = board_window;
    s.queue_limit = params.limit;
    s.queue_ready = queue_ready;
    s.queue.reserve(queue.size());
    for (const Waiting &w : queue) s.queue.push_back(w.number);
    s.clerks.resize(clerks.size());
    for (int i = 0; i < clerks.size(); i++) {
        s.clerks[i].phase = clerks[i].phase;
        s.clerks[i].client = clerks[i].client;
        s.clerks[i].until = clerks[i].until;
    }
    s.served = served;
    s.lost = lost;
    s.lost_full = lost_full;
    s.lost_close = lost_close;
    s.queue_min = queue_min;
    s.queue_max = queue_max;
    if (open_ms > 0) {
        s.queue_avg = double(sum_queue) / open_ms;
        s.occupancy = double(sum_busy) / (clerks.size() * open_ms);
    }
    if (wait_n > 0) s.wait_avg_min = double(sum_wait) / wait_n / MinuteMs;
    s.income = income;
    s.salary = salary;
    s.profit = income - salary;
    s.log.assign(logs.begin(), logs.end());
    return s;
}
