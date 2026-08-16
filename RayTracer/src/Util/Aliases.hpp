#include <cstdint>

using i32 = std::int32_t;
using i64 = std::int64_t;

using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Should never happen but kept incase of stupid edits / end of the world
static_assert(sizeof(float) == 4, "Util/Aliases.hpp requires floats to be 4 bytes (32 bits)!!!");
static_assert(sizeof(double) == 8, "Util/Aliases.hpp requires doubles to be 8 bytes (64 bites)!!!");

using f32 = float;
using f64 = double;
