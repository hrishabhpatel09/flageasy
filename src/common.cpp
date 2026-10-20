// flageasy — version and enum-to-string helpers.

#include "internal.hpp"

namespace flageasy {

const char* library_version() noexcept { return "1.0.0"; }
int library_version_major() noexcept { return 1; }
int library_version_minor() noexcept { return 0; }
int library_version_patch() noexcept { return 0; }

std::string_view to_string(ValueType type) noexcept {
    switch (type) {
        case ValueType::Switch: return "switch";
        case ValueType::Integer: return "int";
        case ValueType::Double: return "double";
        case ValueType::String: return "string";
    }
    return "unknown";
}

std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::None: return "none";
        case ErrorCode::UnknownFlag: return "unknown_flag";
        case ErrorCode::MissingValue: return "missing_value";
        case ErrorCode::InvalidValue: return "invalid_value";
        case ErrorCode::MissingRequired: return "missing_required";
        case ErrorCode::UnexpectedArgument: return "unexpected_argument";
    }
    return "unknown";
}

}  // namespace flageasy
