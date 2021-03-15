

#ifndef PS_SCORE_H
#define PS_SCORE_H

#include <cstdint>

namespace popstar
{

    using Score = int16_t;

    constexpr Score get_bonus_score( int n )
    {
        constexpr Score lookup[] = { 2000, 1980, 1920, 1820, 1680,
                                   1500, 1280, 1020, 720,  380 };

        if ( n >= 10 )
            return 0;
        else
            return lookup[ n ];
    }

    constexpr Score get_score( int n )
    {
        constexpr Score lookup[] = {
            0,    0,    20,   45,   80,   125,  180,  245,  320,  405,  500,
            605,  720,  845,  980,  1125, 1280, 1445, 1620, 1805, 2000, 2205,
            2420, 2645, 2880, 3125, 3380, 3645, 3920, 4205, 4500 };
        if ( n > 30 )
            return 5 * n * n;
        else
            return lookup[ n ];
    }

    constexpr Score get_bonus_score_3x( int n )
    {
        constexpr Score lookup[] = {
            2000, 2000, 2000, 1980, 1980, 1980, 1920, 1920, 1920, 1820,
            1820, 1820, 1680, 1680, 1680, 1500, 1500, 1500, 1280, 1280,
            1280, 1020, 1020, 1020, 720,  720,  720,  380,  380,  380 };
        if ( n >= 30 )
            return 0;
        else
            return lookup[ n ];
    }
    
    constexpr Score get_score_3x( int n )
    {
        constexpr Score lookup[] = {
            0,    0,    0,    0,    0,    0,    20,   20,   20,   45,   45,
            45,   80,   80,   80,   125,  125,  125,  180,  180,  180,  245,
            245,  245,  320,  320,  320,  405,  405,  405,  500,  500,  500,
            605,  605,  605,  720,  720,  720,  845,  845,  845,  980,  980,
            980,  1125, 1125, 1125, 1280, 1280, 1280, 1445, 1445, 1445, 1620,
            1620, 1620, 1805, 1805, 1805, 2000, 2000, 2000, 2205, 2205, 2205,
            2420, 2420, 2420, 2645, 2645, 2645, 2880, 2880, 2880, 3125, 3125,
            3125, 3380, 3380, 3380, 3645, 3645, 3645, 3920, 3920, 3920, 4205,
            4205, 4205, 4500, 4500, 4500, 4805, 4805, 4805, 5120, 5120, 5120,
            5445, 5445, 5445, 5780, 5780, 5780, 6125, 6125, 6125, 6480, 6480,
            6480, 6845, 6845, 6845, 7220, 7220, 7220, 7605, 7605, 7605, 8000,
            8000, 8000 };

        if ( n > 120 )
        {
            n /= 3;
            return 5 * n * n;
        }
        else
            return lookup[ n ];
    }
}  // namespace popstar
#endif