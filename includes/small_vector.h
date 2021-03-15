#ifndef SMALLVECTOR_H
#define SMALLVECTOR_H

#include <cstring>
#include <iterator>
#include <algorithm>

namespace sv
{
    template <typename T, int StructSize_ = 16>
    struct small_vector
    {
        constexpr static auto StructSize = StructSize_;

        static_assert( StructSize == 16 || StructSize == 24 );

        using SizeType = std::conditional_t<StructSize == 16, unsigned int, unsigned long long>;

        constexpr static auto ElementSize = sizeof( T );
        constexpr static auto InternalCapacity = ( StructSize - 1 ) / ElementSize;
        
        static_assert( InternalCapacity > 0, "Element Size Exceeds Internal Buffer" );

        using pointer = T*;

        union
        {
            bool UsingExternal : 1;
            struct
            {
                SizeType : 1, Size : 8 * sizeof( SizeType ) - 1;
                SizeType Capacity;
                T* ExternalBuffer;
            };

            struct
            {
                unsigned char : 1, InternalSize : 7;
                T InternalBuffer[ InternalCapacity ];
            };
        };

        void write_to( void* d, int n ) const { memcpy( d, begin(), n * ElementSize ); }

        small_vector()
        {
            Size = 0;
            UsingExternal = false;
            //memset( this, 0, StructSize );
        }

        ~small_vector()
        {
            if ( UsingExternal ) delete[] ExternalBuffer;
        }

        SizeType size() const { return UsingExternal ? Size : InternalSize; }
        
        SizeType capacity() const { return UsingExternal ? Capacity : InternalCapacity; }

        bool empty() const { return size() == 0; }

        T* begin() const { return UsingExternal ? ExternalBuffer : (T*)InternalBuffer; }

        T* end() const { return begin() + size(); }

        T& operator[]( int n ) { return begin()[ n ]; }

        T* allocate( SizeType n ) { return new T[ n ]; }

        void reallocate( SizeType n )
        {
            if ( UsingExternal )
                delete[] ExternalBuffer;
            else
                UsingExternal = true;
            ExternalBuffer = allocate( n );
            Capacity = n;
        }

        void reserve( SizeType n )
        {
            if ( n <= capacity() ) return;

            auto NewBuffer = allocate( n );

            if ( UsingExternal )
            {
                write_to( NewBuffer, size() );
                delete[] ExternalBuffer;
            }
            else
            {
                write_to( NewBuffer, InternalCapacity );
                //Size = InternalSize;
                Size &= 0xFF;
                UsingExternal = true;
            }
            ExternalBuffer = NewBuffer;
            Capacity = n;
        }

        template<int N>
        auto& assign_imp( const small_vector<T,N>& Source )
        {
            Size = Source.size(); // its fine to corrupt internal buffer here
            if ( capacity() < Size ) reallocate( Size );
            Source.write_to( begin(), Size );
            return *this;
        }

        template <int N>
        auto& operator=( const small_vector<T, N>& Source ) { return assign_imp( Source ); }
        auto& operator=( const small_vector      & Source ) { return assign_imp( Source ); } // damn default operator=

        void push_back( T NewElement )
        {
            if ( size() >= capacity() ) reserve( capacity() * 2 );
            *end() = NewElement;
            ++Size;
        }
        
        void operator+=( T NewElement ) { push_back( NewElement ); }
        
        void pop_back()
        {
            if ( size() ) --Size;
        }

        void erase_every(const T TargetValue)
        {
            Size -= end() - std::partition( begin(), end(), [ TargetValue ]( T Value ) { return Value != TargetValue; } );
            //while (n-->0) pop_back();
        }
    };

    namespace debug
    {
    
        template <typename T>
        void DumpBinary( T& src, int bytes = 1 )
        {
            bytes *= sizeof( T );

            constexpr int ColumnCount = 8;

            if ( bytes > ColumnCount && bytes % ColumnCount > 0 )
                for ( int padding = ColumnCount - bytes % ColumnCount; padding-- > 0; )
                    printf( "         " );

            auto Byte = (unsigned char*)( &src );
            for ( int i = bytes; i-- > 0; )
            {
                for ( int bit = 8; bit-- > 0; ) putchar( Byte[ i ] >> bit & 1 ? '1' : 'o' );
                putchar(' ');
                if ( i % ColumnCount == 0 ) printf( "[%3u ]\n", i );
            }
            putchar( '\n' );
        }
    
        struct dump {
        
            auto& operator<<( const char* str )
            {
                printf( "%s", str );
                return *this;
            }

            template <typename T,int N>
            auto& operator<<( const small_vector<T, N>& Source )
            {
                printf("[ Small Vector Content ]   Size: %i   Capacity: %i \n", Source.size(), Source.capacity() );
                DumpBinary(Source);
                if ( Source.UsingExternal )
                {
                    puts("[ External Content ]");
                    DumpBinary( Source.ExternalBuffer[ 0 ], Source.capacity() );
                }
                return *this;
            }
        };
        
        
    }  // namespace debug

}  // namespace sv



#endif

