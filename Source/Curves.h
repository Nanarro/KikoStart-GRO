#pragma once

#include <algorithm>
#include <array>
#include <cmath>

// 16 volume-envelope shapes. Every curve maps x in [0,1] -> gain in [0,1]
// and ends at 1.0 so that the cycle always loops seamlessly.
namespace kg
{
constexpr int numCurves = 16;

inline const char* const curveNames[numCurves] = {
    "Ramp", "Ease In", "Ease Out", "S-Curve",
    "Hard S", "Late S", "Gate", "Soft Duck",
    "Light Duck", "Dip", "V", "Late Drop",
    "Double Dip", "Stairs", "Snap", "Reverse"
};

inline float smootherstep (float x)
{
    x = std::clamp (x, 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

inline float curveRaw (int idx, float x)
{
    constexpr float pi = 3.14159265358979f;
    x = std::clamp (x, 0.0f, 1.0f);

    switch (idx)
    {
        case 0:  return x;
        case 1:  return x * x;
        case 2:  { const float a = 1.0f - x; return 1.0f - a * a * a; }
        case 3:  return smootherstep (x);
        case 4:
        {
            auto f = [] (float v) { return 1.0f / (1.0f + std::exp (-12.0f * (v - 0.5f))); };
            return (f (x) - f (0.0f)) / (f (1.0f) - f (0.0f));
        }
        case 5:  return smootherstep ((x - 0.30f) / 0.70f);
        case 6:  return smootherstep ((x - 0.45f) / 0.15f);
        case 7:  return 0.40f + 0.60f * smootherstep (x);
        case 8:  return 0.60f + 0.40f * x * x;
        case 9:  { const float s = std::sin (pi * x); return 1.0f - s * s; }
        case 10: return std::fabs (1.0f - 2.0f * x);
        case 11:
            if (x < 0.20f) return 1.0f;
            if (x < 0.30f) return 1.0f - (x - 0.20f) / 0.10f;
            return smootherstep ((x - 0.30f) / 0.70f);
        case 12: return 1.0f - 0.5f * (1.0f - std::cos (4.0f * pi * x));
        case 13: return std::min (1.0f, std::floor (x * 4.0f) / 3.0f);
        case 14: return (1.0f - std::exp (-6.0f * x)) / (1.0f - std::exp (-6.0f));
        case 15:
            if (x < 0.90f) return 1.0f - std::pow (x / 0.90f, 1.5f);
            return (x - 0.90f) / 0.10f;
        default: return 1.0f;
    }
}

constexpr int lutSize = 2048;

// Pre-computed tables so the audio thread never calls exp/pow/sin per sample.
inline const std::array<float, lutSize + 1>& lut (int idx)
{
    static const auto tables = []
    {
        std::array<std::array<float, lutSize + 1>, numCurves> t {};
        for (int i = 0; i < numCurves; ++i)
            for (int j = 0; j <= lutSize; ++j)
                t[(size_t) i][(size_t) j] = curveRaw (i, (float) j / (float) lutSize);
        return t;
    }();
    return tables[(size_t) std::clamp (idx, 0, numCurves - 1)];
}

inline float curveAt (int idx, float x)
{
    const auto& l = lut (idx);
    x = std::clamp (x, 0.0f, 1.0f);
    const float pos = x * (float) lutSize;
    const int i = std::min ((int) pos, lutSize - 1);
    const float frac = pos - (float) i;
    return l[(size_t) i] + frac * (l[(size_t) i + 1] - l[(size_t) i]);
}

// u      : position inside the cycle relative to the curve start (1.0 == whole cycle)
// length : fraction of the cycle that the curve occupies (0.05 .. 1)
inline float gainAt (int idx, float u, float length)
{
    if (u < 0.0f) return 1.0f;
    const float x = u / length;
    if (x >= 1.0f) return 1.0f;
    return curveAt (idx, x);
}
} // namespace kg
