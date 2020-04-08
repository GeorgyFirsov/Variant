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