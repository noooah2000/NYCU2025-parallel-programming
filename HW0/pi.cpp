#include <iostream>
#include <random>
#include <cmath>

int main() {
    const long long number_of_tosses = 9999999;
    long long number_in_circle = 0;

    std::random_device rd;  
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<double> dist(-1.0, 1.0); 

    for (long long toss = 0; toss < number_of_tosses; ++toss) {
        double x = dist(gen);
        double y = dist(gen);
        double distance_squared = x * x + y * y;

        if (distance_squared <= 1.0)
            number_in_circle++;
    }

    double pi_estimate = 4.0 * number_in_circle / static_cast<double>(number_of_tosses);
    std::cout << pi_estimate << std::endl;

    return 0;
}