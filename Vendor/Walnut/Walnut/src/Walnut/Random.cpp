#include "Random.h"
#include <random>

namespace Walnut
{

// this queries the CPU to seed a random device, which is slow, but allows this to be compiled by MinGW GCC
std::mt19937 &Random::s_RandomEngine()
{
    thread_local std::mt19937 generator{ std::random_device{}() };
    return generator;
}

std::uniform_int_distribution<std::mt19937::result_type> Random::s_Distribution;

} // namespace Walnut
