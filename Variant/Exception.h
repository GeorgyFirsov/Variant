#pragma once

// STL headers
#include <stdexcept>


namespace variant {
namespace exception {

    class BadVariantAccess final
        : std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Bad variant access";
        }
    };

} // namespace exception
} // namespace variant