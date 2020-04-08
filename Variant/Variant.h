#pragma once

// STL headers
#include <type_traits>

// C-style headers
#include <cstddef>

// Library headers
#include "TypeList.h"


namespace variant {

    template<
        typename... Types /* Alternatives to store */
    > class Variant
    {
        // Check at compile type if Variant contains at least one type
        static_assert( sizeof...( Types ) > 0, "Variant must be instantiated with at least one type." );

        // Type list that helps to enumerate types we store in variant
        using type_list = utils::TypeList<Types...>;

        // Number of types stored in Variant and maximal size between all stores types
        static constexpr std::size_t types_count = utils::Size( type_list{} );
        static constexpr std::size_t data_size = std::max( { sizeof( Types )... } );

        // Type of container with data stored inside. Must be initialized
        // below constants (it's not beautiful...), because its type
        // is dependent on constant.
        using data_type = typename std::aligned_union<data_size, Types...>::type;

    public:
        // Default constructor, that constructs the first type
        // by calling its default constructor or initializes
        // it with its default value in case of trivial type.
        // It is noexcept if and only if the first type is
        // nothrow default constructible.
        Variant() noexcept( std::is_nothrow_default_constructible<
            decltype( utils::Get<0>( type_list{} ) )::type
        >::value )
            : m_index( 0 )
            , m_data( {} )
        {
            using FirstTypeInVariant = decltype( utils::Get<0>( type_list{} ) )::type;

            new (&m_data) FirstTypeInVariant{};
        }

        Variant( const Variant& other )
        {
            // Not implemented yet
        }

        Variant( Variant&& other ) noexcept( /* Not implemented */ false )
        {
            // Not implemented yet
        }


        // Assigns a value to variant with strict type checking
        template<typename Type>
        Variant& operator=( Type&& value )
        {
            constexpr std::size_t index_of_type = utils::IndexOf<Type>( type_list{} );

            static_assert( 
                index_of_type != types_count,
                "Type of the value you try to assign must be present in the variant"
            );

            using TypeInVariant = decltype( utils::Get<index_of_type>( type_list{} ) )::type;

            // I use placement new and put all data into aligned_uniun, that
            // will automatically release data (there is no dynamic allocation actually).
            new (&m_data) TypeInVariant( value );

            // Set index of current active type
            m_index = index_of_type;

            return *this;
        }


        // Returns zero-based index of current active type
        constexpr std::size_t Index() { return m_index; }

    private:
        std::size_t m_index; // Index of current active type
        data_type   m_data;  // Storage with data
    };

} // namespace variant