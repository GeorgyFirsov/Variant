#include "pch.h"


TEST(Variant, Assign)
{
    variant::Variant<int, char, std::string> var;

    var = 5;

    EXPECT_EQ( var.Index(), 0 );

    var = 'c';

    EXPECT_EQ( var.Index(), 1 );

    var = "String";

    EXPECT_EQ( var.Index(), 2 );
}

TEST(Variant, Copy)
{
    variant::Variant<int, char, std::string> var1;
    
    var1 = "String";

    EXPECT_EQ( var1.Index(), 2 );

    variant::Variant<int, char, std::string> var2( var1 );

    EXPECT_EQ( var2.Index(), 2 );
}

TEST(Variant, Cleanup)
{
    variant::Variant<int, char, double> var;

    var = 5;

    // Must not throw or fail on destruction
}

TEST(Variant, Get)
{
    variant::Variant<int, char, std::string> var;

    var = 5;

    EXPECT_EQ( variant::Get<int>( var ), 5 );
    EXPECT_EQ( variant::Get<int>( std::move( var ) ), 5 );

    var = "String";

    EXPECT_EQ( variant::Get<std::string>( var ), std::string( "String" ) );
    EXPECT_EQ( variant::Get<std::string>( std::move( var ) ), std::string( "String" ) );
}