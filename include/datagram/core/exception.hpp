#pragma once

#include <stdexcept>

namespace datagram {

    // Custom exception type for datagram errors
    struct DatagramException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    // Exception throwing helper - supports no-exceptions mode
    template <typename E> void throw_exception(E &&e) {
#if !defined(__cpp_exceptions) || __cpp_exceptions < 199711L
        std::abort();
#else
        throw e;
#endif
    }

} // namespace datagram
