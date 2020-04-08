#pragma once

// STL headers
#include <type_traits>

// C-style headers
#include <cstddef>

// Library headers
#include "TypeList.h"
#include "Exception.h"


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
        Variant() noexcept( std::is_nothrow_default_constructible<decltype( utils::Get<0>( type_list{} ) )::type>::value )
            : m_valueless( false )
            , m_index( 0 )
            , m_data( {} )
        {
            using FirstTypeInVariant = decltype( utils::Get<0>( type_list{} ) )::type;

            new (&m_data) FirstTypeInVariant{};
        }

        Variant( const Variant& other )
        {
            static_assert( 
                std::is_same<type_list, std::decay<decltype( other )>::type::type_list>::value,
                "Variants must have same internal types here" 
            );

            // In case of constructing from valueless variant currnet
            // variant will be valueless too.
            if (other.m_valueless) {
                m_valueless = true;
                return;
            }

            std::size_t current_index = 0;
            std::size_t required_index = other.Index();

            auto _Assign = [&]( const auto TypeWrapper ) {
                using CurrentType = decltype( TypeWrapper )::type;

                if (current_index++ == required_index) {
                    new (&m_data) CurrentType( *reinterpret_cast<const CurrentType*>( &other.m_data ) );
                }
            };

            utils::ForEach( type_list{}, _Assign );

            m_index = other.m_index;
        }

        Variant( Variant&& other )
        {
            static_assert( 
                std::is_same<type_list, std::decay<decltype( other )>::type::type_list>::value,
                "Variants must have same internal types here" 
            );

            std::size_t current_index = 0;
            std::size_t required_index = other.Index();

            auto _Assign = [&]( auto TypeWrapper ) {
                using CurrentType = decltype( TypeWrapper )::type;

                if (current_index++ == required_index) {
                    new (&m_data) CurrentType( std::move( *reinterpret_cast<CurrentType*>( &other.m_data ) ) );
                }
            };

            utils::ForEach( type_list{}, _Assign );

            m_index = other.m_index;

            m_valueless = false;
            other.m_valueless = true;
        }


        ~Variant() { Cleanup(); }


        // Assigns a value to variant with strict type checking
        template<typename Type>
        Variant& operator=( Type&& value )
        {
            Assign( std::forward<Type>( value ) );

            return *this;
        }


        // Returns zero-based index of current active type
        constexpr std::size_t Index() const noexcept { return m_index; }

    public:

        // lvalue-reference 'Get' overload
        template<typename Type, typename... _Types>
        friend constexpr Type& Get( Variant<_Types...>& );

        // lvalue-reference to constant 'Get' overload
        template<typename Type, typename... _Types>
        friend constexpr const Type& Get( const Variant<_Types...>& );

        // rvalue-reference 'Get' overload
        template<typename Type, typename... _Types>
        friend constexpr Type&& Get( Variant<_Types...>&& var );

        // rvalue-reference to constant 'Get' overload
        template<typename Type, typename... _Types>
        friend constexpr const Type&& Get( const Variant<_Types...>&& var );

        template<typename Type, typename... _Types>
        friend constexpr void GetCheck( const Variant<_Types...>& var );

    private:

        // Puts a value to variant with type checking
        template<typename Type>
        void Assign( Type&& value )
        {
            constexpr std::size_t index_of_type = utils::IndexOf<Type>( type_list{} );

            static_assert( 
                index_of_type != types_count,
                "Type of the value you try to assign must be present in the variant"
            );

            Cleanup();

            // May differ from 'Type'!
            using TypeInVariant = decltype( utils::Get<index_of_type>( type_list{} ) )::type;

            // I use placement new and put all data into aligned_union, that
            // will automatically release data (there is no dynamic allocation actually).
            new (&m_data) TypeInVariant( std::forward<Type>( value ) );

            // Set index of current active type
            m_index = index_of_type;

            m_valueless = false;
        }

        void Cleanup()
        {
            // Traverse all types in typelist to find an active type and then
            // call its destructor if necessary
            std::size_t current_index = 0;

            auto CleanupWorker = [&] ( const auto TypeWrapper ) {
                using CurrentType = std::remove_cv<decltype( TypeWrapper )::type>::type;

                if (current_index++ == m_index && !std::is_trivially_destructible<CurrentType>::value) {
                    reinterpret_cast<CurrentType*>( &m_data )->~CurrentType();
                }
            };

            utils::ForEach( type_list{}, CleanupWorker );
        }

    private:

        bool        m_valueless; // Flag of presense of a value
        std::size_t m_index;     // Index of current active type
        data_type   m_data;      // Storage with data
    };

    
    // class-placeholder that must be place to the first position
    // in variant with only non-default constructible types
    class Monostate final { };


    // std::get analogue for Variant. It is implemented for lvalue-reference,
    // lvalue-reference to constant, rvalue-reference and rvalue-reference to constant.
    
    // Helper function-template checks for correctnes of 'Get' operation
    template<typename Type, typename... _Types>
    constexpr void GetCheck( const Variant<_Types...>& var )
    {
        constexpr std::size_t type_index = utils::IndexOf<Type>( Variant<_Types...>::type_list{} );

        // First of all type must be present in the variant
        static_assert(
            type_index != sizeof...( _Types ),
            "Type of the value you try to obtain must be present in the variant"
        );

        // If type is present, but is inactive, it throws an exception
        if (type_index != var.Index()) {
            throw exception::BadVariantAccess{};
        }
    }

    // 'Get' overrides for listed above types
    template<typename Type, typename... _Types>
    constexpr Type& Get( Variant<_Types...>& var )
    {
        GetCheck<Type>( var );
        return static_cast<Type&>( *reinterpret_cast<Type*>( &var.m_data ) );
    }
    
    template<typename Type, typename... _Types>
    constexpr const Type& Get( const Variant<_Types...>& var )
    {
        GetCheck<Type>( var );
        return static_cast<const Type&>( *reinterpret_cast<const Type*>( &var.m_data ) );
    }

    template<typename Type, typename... _Types>
    constexpr Type&& Get( Variant<_Types...>&& var )
    {
        GetCheck<Type>( var );
        return static_cast<Type&&>( *reinterpret_cast<Type*>( &var.m_data ) );
    }
    
    template<typename Type, typename... _Types>
    constexpr const Type&& Get( const Variant<_Types...>&& var )
    {
        GetCheck<Type>( var );
        return static_cast<const Type&&>( *reinterpret_cast<const Type*>( &var.m_data ) );
    }

} // namespace variant