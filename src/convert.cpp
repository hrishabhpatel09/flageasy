// flageasy — string-to-value conversion helpers.
// These are used by the parser (validation) and by ParseResult::as<T>().

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <string_view>
#include <system_error>

#include "internal.hpp"

namespace flageasy {
namespace detail {

bool convert_int(const std::string& text, long long& out, std::string& error) {
    const auto fail = [&]() {
        error = "invalid integer value '" + text + "'";
        return false;
    };

    std::string_view s(text);
    if (!s.empty() && s.front() == '+') s.remove_prefix(1);
    if (s.empty()) return fail();
    if (std::isspace(static_cast<unsigned char>(s.front()))) return fail();

    long long value = 0;
#if FLAGEASY_HAS_TO_CHARS
    const char* first = s.data();
    const char* last = s.data() + s.size();
    const std::from_chars_result result = std::from_chars(first, last, value, 10);
    if (result.ec != std::errc() || result.ptr != last) return fail();
#else
    errno = 0;
    const std::string copy(s);
    char* end = nullptr;
    value = std::strtoll(copy.c_str(), &end, 10);
    if (end == copy.c_str() || end == nullptr || *end != '\0' || errno == ERANGE)
        return fail();
#endif

    out = value;
    return true;
}

bool convert_double(const std::string& text, double& out, std::string& error) {
    const auto fail = [&]() {
        error = "invalid number '" + text + "'";
        return false;
    };

    std::string_view s(text);
    if (s.empty()) return fail();
    if (std::isspace(static_cast<unsigned char>(s.front()))) return fail();

    double value = 0.0;
#if FLAGEASY_HAS_TO_CHARS
    const char* first = s.data();
    const char* last = s.data() + s.size();
    const std::from_chars_result result = std::from_chars(first, last, value);
    if (result.ec != std::errc() || result.ptr != last) return fail();
#else
    errno = 0;
    const std::string copy(s);
    char* end = nullptr;
    value = std::strtod(copy.c_str(), &end);
    if (end == copy.c_str() || end == nullptr || *end != '\0') return fail();
    if (errno == ERANGE && value == 0.0) return fail();  // underflow to zero
#endif

    out = value;
    return true;
}

bool convert_bool(const std::string& text, bool& out, std::string& error) {
    const std::string lower = internal::to_lower(text);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        out = true;
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        out = false;
        return true;
    }
    error = "invalid boolean value '" + text +
            "' (expected: true/false, yes/no, on/off, 1/0)";
    return false;
}

}  // namespace detail
}  // namespace flageasy
