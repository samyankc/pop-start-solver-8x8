#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <x86intrin.h>
#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <mutex>
#include <algorithm>

#include "includes/index_range.h"
#include "includes/small_vector.h"
#include "includes/pop_star_score.h"
#include "includes/constants.h"

using namespace std;
using namespace popstar;
using namespace index_range;
using sv::small_vector;

constexpr auto All = Range( MAX_x );

//****************************************************************************//
//************************ Important Helper Function  ************************//
//****************************************************************************//

uint32_t block_pext_u32( const uint32_t _src, const uint32_t _excluder )
{
    uint32_t __result = _src;

    uint32_t __excluder = _excluder;
    uint32_t __extract;
    uint32_t __new_pos;

    while ( __excluder )
    {
        __new_pos = 32 - __builtin_clz( __excluder );
        __extract = __bextr_u32( __result, __new_pos | 0x2000 );
        __excluder = _bzhi_u32( ~__excluder, __new_pos );

        __new_pos = 32 - __builtin_clz( __excluder );
        __result = __extract << __new_pos | _bzhi_u32( __result, __new_pos );
        __excluder = _bzhi_u32( ~__excluder, __new_pos );
    }

    return __result;
}

int PopCount(const uint64_t Key){ return __builtin_popcountll( Key ); }

//****************************************************************************//
//****************************************************************************//

struct Point
{
    uint8_t x : 4, y : 4;
    
    friend bool operator==( const Point& lhs, const Point& rhs )
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    friend ostream& operator<<( ostream& out, const Point& CurrentPoint )
    {
        if ( CurrentPoint.x >= MAX_x || CurrentPoint.y >= MAX_y )
            return out << "Point:[ END POINT ]";
        return out << "Point:[" << (int)CurrentPoint.x << ',' << (int)CurrentPoint.y << "]";
    }
};

struct Future
{
    constexpr static auto NoMove = Point( MAX_x, MAX_y );
    constexpr static auto Explored = NoMove;
    constexpr static Score PendingScore = -1;
    
    Score BestScore{ PendingScore };
    Point BestMove{ NoMove };
    
    void operator|=( Future AnotherFuture )
    {
        if ( AnotherFuture.BestScore > BestScore ) *this = AnotherFuture;
    }

};

namespace Operational
{
    struct Puzzle
    {
        static Puzzle MasterPuzzle;

        uint32_t Column[ MAX_x ]{ 0 };

        union
        {
            uint64_t Key;
            uint8_t KeepMap[ 8 ];
        };
                
        Puzzle() = default;
        Puzzle(const Puzzle&) = default;

        uint32_t at( uint32_t x, uint32_t y ) const
        {
            #ifdef _X86INTRIN_H_INCLUDED  //_BMIINTRIN_H_INCLUDED
                return __bextr_u32( Column[ x ], BEXTR_SHIFT[ y ] );
                // == _bextr_u32(Column[x],SHIFT[y],3); == bextr(_src, _start | _len(=3) << 8)
            #else
                return Column[ x ] >> SHIFT[ y ] & TRIPLET_MASK;
            #endif
        }

        auto operator()( uint32_t x, uint32_t y ) const { return at( x, y ); }

        void Clear()
        {
            memset( Column, 0, MAX_x * sizeof( uint32_t ) );
            Key = 0;
        }

        void Clear( uint32_t x, uint32_t y ) { Column[ x ] &= ~TRI_MASK[ y ]; } // == TRI_MASK_FLIP[y]
        void Fill( uint32_t x, uint32_t y ) { Column[ x ] |= TRI_MASK[ y ]; }
        void Fill( uint32_t x, uint32_t y, uint32_t value )
        {
            Clear( x, y );
            Column[ x ] |= value << SHIFT[ y ];
        }
        
        bool Linked( uint32_t x, uint32_t y ) const
        {
            if ( at( x, y ) == 0 ) return false;
            if ( x == MAX_x - 1 )
                 return at( x, y ) == at( x, y + 1 );
            else return at( x, y ) == at( x, y + 1 ) || at( x, y ) == at( x + 1, y );
        }
        
        int CountCell() const { return PopCount( Key ); }

        Puzzle FloodFill( int x, int y )
        {
            Puzzle FootPrint;
            struct
            {
                Puzzle& FootPrint;
                Puzzle& Target;
                uint32_t TargetColour;
                void operator()( int x, int y )
                {
                    if ( x < 0 || x >= MAX_x || y < 0 || y >= MAX_y || TargetColour != Target( x, y ) )
                        return;

                    FootPrint.Fill( x, y );
                    Target.Clear( x, y );
                    ( *this )( x+1, y   );
                    ( *this )( x-1, y   );
                    ( *this )( x  , y+1 );
                    ( *this )( x  , y-1 );
                }
            } RecursiveFloodFill{ FootPrint, *this, at( x, y ) };
            RecursiveFloodFill( x, y );
            return FootPrint;
        }
        
        void FastFloodFill( int x, int y )
        {
            struct
            {
                Puzzle& Target;
                uint32_t TargetColour;
                void operator()( int x, int y )
                {
                    if ( x < 0 || x >= MAX_x || y < 0 || y >= MAX_y || TargetColour != Target( x, y ) )
                        return;
                    Target.Clear( x, y );
                    ( *this )( x+1, y   );
                    ( *this )( x-1, y   );
                    ( *this )( x  , y+1 );
                    ( *this )( x  , y-1 );
                }
            } RecursiveFloodFill{ *this, at( x, y ) };
            RecursiveFloodFill( x, y );
        }

        void ColumnShrink()
        {
            int space = 0;
            for ( ; Column[ space ] && ++space < MAX_x; ) {}  // short circuit
            for ( int stuff = space; ++stuff < MAX_x; )
            {
                if ( Column[ stuff ] )
                {
                    Column[ space++ ] = Column[ stuff ];
                    Column[ stuff ] = 0;
                }
            }
        }

        void Eliminate( const Puzzle& FloodFillFootPrint )
        {
            int x = 0;
            while ( !FloodFillFootPrint.Column[ x ] ) ++x;  //must be in range, so no boundary check
            while ( FloodFillFootPrint.Column[ x ] && x < MAX_x )
            {
                Column[ x ] = block_pext_u32( Column[ x ], FloodFillFootPrint.Column[ x ] );
                ++x;
            }
            ColumnShrink();
            Compress();
        }

        auto Options() const
        {
            small_vector<Point,16> OptionList;
            auto OptionPuzzle = *this;
            for ( auto x : All ) 
                for ( auto y : All )
                    if ( OptionPuzzle.Linked( x, y ) )
                    {
                        OptionList += Point( x, y );
                        OptionPuzzle.FastFloodFill( x, y );
                    }
            return OptionList;
        }
        
        uint64_t Compress()
        {
            auto RetrieveKeepMap = []( const uint32_t MasterColumn, const uint32_t TargetColumn ) -> uint8_t
            {
                if ( TargetColumn > 1 << SHIFT[ MAX_y - 1 ] )
                {
                    if ( MasterColumn == TargetColumn ) return 0b11111111;
                    return 0; // full column but no match
                }
                auto at = []( const uint32_t Column, const int Pos ) { return __bextr_u32( Column, BEXTR_SHIFT[ Pos ] ); };
                
                for ( uint8_t Result = 0, y = 0; auto Master_y : All ) 
                {
                    if ( at( TargetColumn, y ) == at( MasterColumn, Master_y ) ) 
                    {
                        Result |= 1 << Master_y;
                        if ( at( TargetColumn, ++y ) == 0 ) return Result;
                    }
                }
                return 0;
            };
            
            Key = 0;
            for ( auto x = 0; auto Master_x : All ) 
            {
                KeepMap[ x ] = RetrieveKeepMap( MasterPuzzle.Column[ Master_x ], Column[ x ] );
                if ( KeepMap[ x ] != 0 ) ++x;
            }
            return Key;
        }

    };

    Puzzle Puzzle::MasterPuzzle;
    
    void operator<<=( Puzzle& CurrentPuzzle, const Puzzle& FloodFillFootPrint )
    {
        CurrentPuzzle.Eliminate( FloodFillFootPrint );
    }

    void operator<<=( Puzzle& CurrentPuzzle, const Point CurrentMove )
    {
        CurrentPuzzle <<= CurrentPuzzle.FloodFill( CurrentMove.x, CurrentMove.y );
    }
    
    auto operator<<( const Puzzle& CurrentPuzzle, const Point CurrentMove )
    {
        auto ResultantPuzzle = CurrentPuzzle;
        ResultantPuzzle <<= CurrentMove;
        return ResultantPuzzle;
    }

    void operator<<( Puzzle& CurrentPuzzle, const char* FileName )
    {
        ifstream Fin( FileName );
        if ( !Fin ) { cout << "Not Found: " << FileName << endl; }
        else
        {
            for ( auto y : All | Reverse() )
            {
                string DataRow;
                getline( Fin, DataRow );
                for ( auto x : All ) CurrentPuzzle.Fill( x, y, DataRow[ x ] - '0' );
            }
            CurrentPuzzle.Compress();
            cout << "Puzzle loaded:\t[" << FileName << "]\n";
        }
    }

    ostream& operator<<( ostream& out, const Puzzle& CurrentPuzzle )
    {
        out << "Key: " <<setw(17)<< hex << uppercase  //
            << CurrentPuzzle.Key << dec << nouppercase << '\t';
        out << "Cell Count: " << CurrentPuzzle.CountCell();
        
        for ( auto y : All | Reverse() )
        {
            out << "\n " << y << "  ";
            for ( auto x : All ) out << (char)(CurrentPuzzle( x, y ) ==0? ' ' : '0'+CurrentPuzzle( x, y ) ) << ' ';
        }
        out << "\n\n   ";
        for ( auto x : All ) out << " " << x;
        out << "\n\n";
        return out;
    }
    
}  // namespace Puzzle

namespace Storage
{
    struct Puzzle
    {
        Puzzle& operator=( const Puzzle& ) = delete;

        uint64_t Key;
        
        Future BestFuture;

        Puzzle( uint64_t Key ) : Key{ Key }{}

        int CountCell() const { return PopCount( Key ); }

        using Bucket = vector<Storage::Puzzle>;
        inline static array<Bucket, HASH_SIZE> Archive;
        inline static array<mutex, PUZZLE_SIZE> DepthLock;
        
    };

    auto Hash( const uint64_t Key ) { return Key % HASH_SIZE; }

    auto Match = []( const uint64_t Key ) {
        return [ Key ]( const auto& Item ) { return Item.Key == Key; };
    };

    bool Contains( uint64_t Key )
    {
        auto& Bucket = Puzzle::Archive[ Hash( Key ) ];
        return any_of( rbegin( Bucket ), rend( Bucket ), Match( Key ) );
    }

    bool RequireManage( uint64_t Key )
    {
        lock_guard Lock{ Puzzle::DepthLock[ PopCount( Key ) ] };
        auto& Bucket = Puzzle::Archive[ Hash( Key ) ];
        if ( any_of( rbegin( Bucket ), rend( Bucket ), Match( Key ) ) )
            return false;
        Bucket.emplace_back( Key );
        return true;
    }
    bool Taken( uint64_t Key ) { return !RequireManage( Key ); }

    struct {
        Puzzle& operator[]( const uint64_t Key ) // assume Storage::Contains(Key)
        {
            auto& Bucket = Puzzle::Archive[ Hash( Key ) ];
            return *find_if( rbegin( Bucket ), rend( Bucket ), Match( Key ) );
        }
    } Proxy;
}  // namespace Storage


//****************************************************************************//
//****************************** Major Function ******************************//
//****************************************************************************//

namespace ThisThread
{
    auto Explore( const Operational::Puzzle& SourcePuzzle );
}

auto ThisThread::Explore( const Operational::Puzzle& SourcePuzzle )
{
    Future ExplorationResult;
    const auto BaselineCellCount = SourcePuzzle.CountCell();
    const auto PuzzleKey = SourcePuzzle.Key;

    if ( Storage::Contains( PuzzleKey ) ||  //
         Storage::Taken( PuzzleKey ) )      // do not change order, rely on short circuit
    {
        return Storage::Proxy[ PuzzleKey ].BestFuture.BestScore;
    }

    auto& RecordProxy = Storage::Proxy[ PuzzleKey ]; // obtain a proxy asap, reduce potential search time?

    if ( auto Options = SourcePuzzle.Options();  //
         Options.empty() )
    {
        ExplorationResult = Future( get_bonus_score( BaselineCellCount ), Future::NoMove );
    }
    else
    {
        while ( !Options.empty() )
        {
            for ( auto& CurrentOption : Options )
            {
                auto VariantPuzzle = SourcePuzzle << CurrentOption;
                auto VariantScore = ThisThread::Explore( VariantPuzzle );
                if ( VariantScore != Future::PendingScore )
                {
                    VariantScore += get_score( BaselineCellCount - VariantPuzzle.CountCell() );
                    ExplorationResult |=  Future( VariantScore, CurrentOption ) ;
                    CurrentOption = Future::Explored;
                }
            }
            Options.erase_every( Future::Explored );
        }
    }
    
    RecordProxy.BestFuture = ExplorationResult;
    return ExplorationResult.BestScore;
}


auto Explore( const Operational::Puzzle& SourcePuzzle )
{
    Future ExplorationResult;
    const auto BaselineCellCount = SourcePuzzle.CountCell();

    atomic<int> AvailableThreads = THREAD_PERMISSION;
    mutex mtx;
    for ( auto CurrentOption : SourcePuzzle.Options() ) 
    {
        while ( AvailableThreads <= 0 ) this_thread::yield();
        --AvailableThreads;
        thread( [ & ] {
            auto VariantPuzzle = SourcePuzzle << CurrentOption;
            auto VariantScore = ThisThread::Explore( VariantPuzzle );
            VariantScore += get_score( BaselineCellCount - VariantPuzzle.CountCell() );
            {
                lock_guard Lock{ mtx };
                ExplorationResult |= Future( VariantScore, CurrentOption );
                ++AvailableThreads;
            }
        } ).detach();
    }
    while ( AvailableThreads < THREAD_PERMISSION ) this_thread::yield();
    return ExplorationResult;

}

//****************************************************************************//
//****************************************************************************//


//****************************************************************************//
//****************************** Data Analysis  ******************************//
//****************************************************************************//

void CheckDistribution( const auto& HashTable, const int SamplingThreshold = 200 )
{
    auto max = [](auto a, auto b){ return std::max<unsigned long long>(a,b); };
    auto min = [](auto a, auto b){ return std::min<unsigned long long>(a,b); };

    constexpr auto BarWidthLimit = 120;
    vector<int> BucketCount( SamplingThreshold + 2 );
    auto Total = 0ull;
    auto SpanMax = 0ull;
    auto SpanMin = ~0ull;
    
    for( const auto& Bucket : HashTable )
    {
        auto BucketSize = Bucket.size();
        Total += BucketSize;
        SpanMax = max( SpanMax, BucketSize );
        SpanMin = min( SpanMin, BucketSize );
        ++BucketCount[ min( BucketSize, SamplingThreshold + 1 ) ];
    }
    auto TruncatedSpanMax = min( SpanMax, SamplingThreshold );

    //Normalization
    auto BucketCountMax = 0ull;
    auto BucketCountMin = ~0ull;
    for ( auto BucketSize : Range( SpanMin, SpanMax ) )
    {
        auto BucketCountNow = BucketCount[ BucketSize ];
        BucketCountMax = max( BucketCountMax, BucketCountNow );
        BucketCountMin = min( BucketCountMin, BucketCountNow );
    }

    auto Scale = [ = ]( auto Orginal ) { return Orginal * BarWidthLimit / ( BucketCountMax - BucketCountMin + 1 ); };

    cout << "[ Bucket Size Distruibution ]  \t";
    cout <<  "Total : " << Total << "  Max : " << SpanMax << "  Min : " << SpanMin <<'\n';
    for ( auto BucketSize : Range( SpanMin, TruncatedSpanMax ) )
        cout << setw( 4 ) << BucketSize << " : "                                         //
             << setfill( '|' ) << setw( 1 + Scale( BucketCount[ BucketSize ] ) ) << ' '  //
             << setfill( ' ' ) << BucketCount[ BucketSize ] << '\n';
    cout << " >" << SamplingThreshold << " :  " << BucketCount[ SamplingThreshold + 1 ] << '\n';    
}

//****************************************************************************//
//****************************************************************************//

int main( int argc, const char* argv[] )
{
    auto& MasterPuzzle = Operational::Puzzle::MasterPuzzle;

    MasterPuzzle << PUZZLE_PATH;

    for ( auto& Bucket : Storage::Puzzle::Archive ) Bucket.reserve( 200 );
    cout << "Allocation Complete\n";

    cout << MasterPuzzle << endl;

    auto ExplorationResult = Explore( MasterPuzzle );

    cout << "\nFinal Score: " << ExplorationResult.BestScore << endl;

    CheckDistribution( Storage::Puzzle::Archive );

    cin.ignore();

    // present solution

    auto CurrentPuzzle = MasterPuzzle;
    for ( auto NextMove = ExplorationResult.BestMove;  //
          NextMove != Future::NoMove;                  //
          CurrentPuzzle <<= NextMove,                  //
          NextMove = Storage::Proxy[ CurrentPuzzle.Key ].BestFuture.BestMove )
    {
        auto Options = CurrentPuzzle.Options();
        cout << CurrentPuzzle << "Picking: " << NextMove;
        cout << " Among " << Options.size() << '\n';
        cin.ignore();
    }

    cout << CurrentPuzzle << "END" << endl;
    cin.ignore();

    for ( auto& Bucket : Storage::Puzzle::Archive ) vector<Storage::Puzzle>().swap( Bucket );
    cout << "Deallocation Complete";
    return 0;
}





