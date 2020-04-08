#pragma once

// STL headers
#include <utility>

// C-style headers
#include <cstddef>


namespace variant {
namespace utils {

    // Light-weight wrapper over any type to make it
    // possible to return it in compile time
    template<
        typename Type /* Type to wrap */
    > struct Identity {
        using type = Type;
    };

    // Helper-template for 'Identity'
    template<
        typename Type /* Type to wrap */
    > using IdenticalType = typename Identity<Type>::type;


    // Typelist is implemented as an empty type to make
    // it possible to pass this class into functions that
    // must work in compile time.
    template<
        typename... Types /* Types in typelist */
    > struct TypeList { /* Nothing to do here */ };


    // Function-template that returns size of typelist
    template<
        typename... Types /* Types in typelist */
    > constexpr std::size_t Size( TypeList<Types...> )
    {
        return sizeof...( Types );
    }


    // Helpers for 'Get' function-template
    namespace {

        template<typename Dummy>
        struct GetImpl; /* Not implemented */

        template<std::size_t... Idxs>
        struct GetImpl<std::index_sequence<Idxs...>> 
        {
            template<typename Type>
            static constexpr Type dummy( 
                // 
                // 'dummy' has sizeof...(Idxs) void* parameters, then Type* and then arbitrary number of arguments 
                //                    |                                 |               |
                //                    |                               |--   |------------
                //                    V                               V     V
                decltype( Idxs, static_cast<void*>( nullptr ) )..., Type*, ...
                //                                                    ^
                // Exactly on the Idx'th (see below) place -----------|
            );  /* Not implemented */
        };

    } // helper anonymous namespace

    // Function-template that returns a wrapper over type
    // on the Idx'th place in type list. It performs boundary
    // checking with raising a static assertion.
    template<
        size_t Idx        /* Index to get type at */,
        typename... Types /* Types stored in typelist */
    > constexpr auto Get( TypeList<Types...> type_list )
    {
        static_assert( Idx < Size( type_list ), "Out of range in Get" );

        return Identity<decltype( 
            GetImpl< std::make_index_sequence<Idx> >::dummy( static_cast<Types*>( nullptr )... ) 
        )>{};
    }


    // Helpers for 'IndexOf' function-template
    namespace {

        template<
            typename       Type  /* Type to get index of */,
            typename...    Types /* Types stored in typelist */,
            std::size_t    Idx   /* First helper index */,
            std::size_t... Idxs  /* Rest helper indices */
        > constexpr std::size_t IndexOfImpl( TypeList<Types...> type_list, std::index_sequence<Idx, Idxs...> )
        {
            constexpr std::size_t current_index = sizeof...( Types ) - sizeof...( Idxs ) - 1;

            using To   = decltype( Get<current_index>( type_list ) )::type;
            using From = Type;

            // If current type in typelist is trivial, I check an equality
            // of types, otherwise checking for ability to construct
            // type in typelist from passed value is performed.
            return std::is_trivial<To>::value 
                ? ( std::is_same<From, To>::value          ? current_index : IndexOfImpl<Type>( type_list, std::make_index_sequence<sizeof...( Idxs )>{} ) )
                : ( std::is_constructible<From, To>::value ? current_index : IndexOfImpl<Type>( type_list, std::make_index_sequence<sizeof...( Idxs )>{} ) );
        }

        template<
            typename    Type  /* Type to get index of */,
            typename... Types /* Types stored in typelist */,
            std::size_t Idx   /* Last helper index */
        > constexpr std::size_t IndexOfImpl( TypeList<Types...> type_list, std::index_sequence<Idx> )
        {
            constexpr std::size_t current_index = sizeof...( Types ) - 1;

            using To   = decltype( Get<current_index>( type_list ) )::type;
            using From = Type;

            // If the last type in typelist is not the type we look for, 
            // I just return the size of typelist
            return std::is_trivial<To>::value 
                ? ( std::is_same<From, To>::value        ? current_index : sizeof...( Types ) )
                : ( std::is_convertible<From, To>::value ? current_index : sizeof...( Types ) );
        }

    } // helper anonymous namespace

    // Function-template that returns index of specified type if it is
    // present in the typelist and is not returns the size of typelist.
    template<
        typename    Type,
        typename... Types
    > constexpr std::size_t IndexOf( TypeList<Types...> type_list )
    {
        return IndexOfImpl<Type>( type_list, std::make_index_sequence<sizeof...( Types )>{} );
    }


    // Helpers for 'ForEach' function-template
    namespace {

        template<typename Fn, typename... Types, std::size_t Idx, std::size_t... Idxs>
        Fn ForEechImpl( TypeList<Types...> type_list, Fn func, std::index_sequence<Idx, Idxs...> )
        {
            constexpr std::size_t current_index = sizeof...( Types ) - sizeof...( Idxs ) - 1;

            func( Get<current_index>( type_list ) );
            return ForEechImpl( type_list, func, std::make_index_sequence<sizeof...( Idxs )>{} );
        }

        template<typename Fn, typename... Types, std::size_t Idx>
        Fn ForEechImpl( TypeList<Types...> type_list, Fn func, std::index_sequence<Idx> )
        {
            constexpr std::size_t current_index = sizeof...( Types ) - 1;

            func( Get<current_index>( type_list ) );
            return func;
        }

    } // helper anonymous namespace

    // Applies func to each type's in typelist wrapper. Useful to enumerate types and
    // use an information about them on each iteration.
    template<typename Fn, typename... Types>
    Fn ForEach( TypeList<Types...> type_list, Fn func ) 
    {
        return ForEechImpl( type_list, func, std::make_index_sequence<sizeof...( Types )>{} );
    }

} // namespace utils
} // namespace variant