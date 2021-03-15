#ifndef POPSTAR_CONST_H
#define POPSTAR_CONST_H

#include <stdint.h>

constexpr auto MAX_x = 8;
constexpr auto MAX_y = 8;
constexpr auto MAX_layer = 40;

constexpr auto PUZZLE_SIZE = MAX_x * MAX_y;

constexpr auto PUZZLE_PATH   = "puzzle.txt";
constexpr auto SOLUTION_PATH = "puzzle_solution.txt";
constexpr auto ARCHIVE_PATH  = "E:\\__PUZZLE_ARCHIVE\\ARCHIVE_";

constexpr auto HASH_SIZE = 700001; // prime option: 99991, 199999, 499099, 599999, 700001, 999979 1999111 2097143 

constexpr auto OPTION_CAPACITY = 16;

constexpr auto THREAD_PERMISSION = 10;

//constexpr auto THRESHOLD = ;


constexpr uint32_t TRIPLET_MASK = 0b111;


#define DISABLE_LOOKUP_TABLE
#ifndef DISABLE_LOOKUP_TABLE
    constexpr uint32_t SHIFT[] = { 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30 };	// lookup table return 3n
    constexpr uint32_t TRI_MASK[] = { 0b111, 0b111<<3, 0b111<<6, 0b111<<9, 0b111<<12, 0b111<<15, 0b111<<18, 0b111<<21, 0b111<<24, 0b111<<27 };
#else
    constexpr struct { constexpr uint32_t operator[]( int n ) const { return 3 * n; } } SHIFT;
    constexpr struct { constexpr uint32_t operator[]( int n ) const { return 0b111 << SHIFT[n]; } } TRI_MASK;
#endif

constexpr uint32_t TRI_MASK_FLIP[] = { ~TRI_MASK[0], ~TRI_MASK[1], ~TRI_MASK[2], ~TRI_MASK[3], ~TRI_MASK[4], ~TRI_MASK[5], ~TRI_MASK[6], ~TRI_MASK[7], ~TRI_MASK[8], ~TRI_MASK[9] };

constexpr uint32_t BEXTR_SHIFT[] = { 0|0x300, 3|0x300, 6|0x300, 9|0x300, 12|0x300, 15|0x300, 18|0x300, 21|0x300, 24|0x300, 27|0x300, 30|0x100 }; // last element allow out of range access and return 0

#endif
