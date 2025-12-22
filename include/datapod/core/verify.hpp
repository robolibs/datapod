#pragma once

#include <string>

#include "datapod/core/exception.hpp"

namespace datapod {

    // Verify condition with C-string message
    inline void verify(bool const condition, char const *msg) {
        if (!condition) {
            throw_exception(DatapodException{msg});
        }
    }

    // Verify condition with std::string message
    inline void verify_str(bool const condition, std::string msg) {
        if (!condition) {
            throw_exception(DatapodException{std::move(msg)});
        }
    }

} // namespace datapod
