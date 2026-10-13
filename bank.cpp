#include "bank.h"

constexpr long long LunchMs = 30 * MinuteMs;
constexpr long long OpenWeekday = 9 * HourMs;
constexpr long long CloseWeekday = 17 * HourMs;
constexpr long long OpenSaturday = 10 * HourMs;
constexpr long long CloseSaturday = 16 * HourMs;
constexpr int SalaryRub = 2000;
constexpr int LogLimit = 400;

Bank::Bank() {

}