#pragma once
#include <random>
#include <chrono>

class Generators {
public:
    Generators() : Generators((long long) std::chrono::steady_clock::now().time_since_epoch().count()) {}
    explicit Generators(long long seed) : rnd(seed) {}

    int randomInt(int a, int b); // [a, b]
    double randomDouble(double a, double b, int k);
    int sample(int min_v, int max_v, bool normal, double mean, double sigma);

private:
    std::mt19937 rnd;
};
