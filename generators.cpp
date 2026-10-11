#include "generators.h"
#include <cmath>
#include <algorithm>

int Generators::randomInt(int a, int b) {
    if (a > b) {
        std::swap(a, b);
    }
    std::uniform_int_distribution<int> dist(a, b);
    return dist(rnd);
}

double Generators::randomDouble(double a, double b, int k) {
    if (a > b) {
        std::swap(a, b);
    }
    long long p = pow(10, k);
    long long l = ceil(a * p), r = floor(b * p);
    if (l > r) {
        r = l;
    }
    std::uniform_int_distribution<long long> dist(l, r);
    return dist(rnd) / (double) p;
}

int Generators::sample(int min_v, int max_v, bool normal, double mean, double sigma) {
    if (min_v > max_v) {
        std::swap(min_v, max_v);
    }
    if (!normal) {
        return randomInt(min_v, max_v);
    }
    if (sigma < 1e-9) {
        int v = llround(mean);
        return std::min(max_v, std::max(min_v, v));
    }
    std::normal_distribution<double> dist(mean, sigma);
    for (int i = 0; i < 10000; i++) {
        double x = dist(rnd);
        if (x >= min_v && x <= max_v) {
            int v = llround(x);
            return std::min(max_v, std::max(min_v, v));
        }
    }
    int v = llround(mean);
    return std::min(max_v, std::max(min_v, v));
}
