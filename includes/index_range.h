#ifndef INDEXRANGE_H
#define INDEXRANGE_H

#include <type_traits>

namespace index_range
{
    template<typename T>
    struct SelfReferencing {
        T self_;
        constexpr T operator*() { return self_; }
        constexpr operator T&(){ return self_; }
        constexpr operator const T&() const { return self_; }
    };

    //template<typename Iter, typename Guide = AdvancementGuide<Iter> >
    template<typename Iter>
    struct ForwardIter
    {
        Iter current_;
        constexpr auto operator+=( auto n ) { return this->current_ += n, *this; }
        constexpr auto operator++() { return *this += +1; }
        constexpr auto operator--() { return *this += -1; }
        /*
        constexpr auto Advance( auto n ) { return current_ += n, *this; }
        constexpr auto operator+=( auto n ) { Advance( n ); }
        constexpr auto operator++() { return Advance( 1 ); }
        constexpr auto operator--() { return Advance( -1 ); }
        */
        constexpr decltype( auto ) operator*() { return *current_; }
        constexpr bool operator!=( const ForwardIter& rhs ) const { return current_ != rhs.current_; }
    };

    template<typename Iter>
    struct ReverseIter : ForwardIter<Iter>
    {
        constexpr ReverseIter( Iter src ) : ForwardIter<Iter>( --src ) {}
        // shadowing parent's overloads
        constexpr auto operator+=( auto n ) { return this->current_ += -n, *this; }
        constexpr auto operator++() { return *this += +1; }
        constexpr auto operator--() { return *this += -1; }
        /*
        using ForwardIter<Iter>::Advance;
        constexpr auto operator+=( auto n ) { Advance( -n ); }
        constexpr auto operator++() { return Advance( -1 ), *this; }
        constexpr auto operator--() { return Advance( 1 ), *this; }
        */
    };

    template<typename Iter>
    struct RangeTemplate
    {
        Iter begin_;
        Iter end_;
        constexpr RangeTemplate( Iter begin__, Iter end__ ) : begin_( begin__ ), end_( end__ ) {}
        constexpr auto begin() const { return begin_; }
        constexpr auto end() const { return end_; }
    };
    
    template<typename Iter> 
    using ForwardRange = RangeTemplate<Iter>;

    template<typename Iter>
    struct ReverseRange : RangeTemplate<ReverseIter<Iter> >
    {
        constexpr ReverseRange( Iter begin_, Iter end_ )
            : RangeTemplate<ReverseIter<Iter> >( end_, begin_ ){}
    };

    //using IndexIter = ForwardIter<SelfReferencing<long long> >;
    template<typename T, typename Iter = ForwardIter<SelfReferencing<T> > >
    struct Range : RangeTemplate<Iter>
    {
        constexpr Range( T first, T last ) : RangeTemplate<Iter>( { first }, { last + 1 } ) {}
        constexpr Range( T distance ) : Range( 0, distance - 1 ) {}
    };

    struct Reverse {};
    struct Drop { long long N = 0; }; 
    struct Take { long long N = 0; };

    template<typename Container, typename Adaptor,
        std::enable_if_t<
            std::is_same_v<Adaptor, Drop> ||
            std::is_same_v<Adaptor, Take> ||
            std::is_same_v<Adaptor, Reverse>
        , bool> = true
    > constexpr auto operator|( Container&& C, Adaptor A ) { return C | A; }

    constexpr auto operator|( auto& Container, Reverse )
    {
        return ReverseRange( Container.begin(), Container.end() );
    }

    constexpr auto operator|( auto& Container, Drop Amount )
    {
        auto new_begin_ = Container.begin();
        auto end_ = Container.end();
        for ( auto i : Range( Amount.N ) )
            if ( new_begin_ != end_ ) ++new_begin_;
        return ForwardRange( new_begin_, end_ );
    }
    
    constexpr auto operator|( auto& Container, Take Amount )
    {
        auto new_begin_ = Container.begin();
        auto end_ = Container.end();
        for ( auto i : Range( Amount.N ) )
            if ( new_begin_ != end_ ) ++new_begin_;
        return ForwardRange( Container.begin(), new_begin_ );
    }

}  // namespace index_range

#endif
