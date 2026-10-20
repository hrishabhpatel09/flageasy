// flageasy — internal definitions shared between translation units.
// Not installed; implementation detail of the library.

#ifndef FLAGEASY_INTERNAL_HPP
#define FLAGEASY_INTERNAL_HPP

#include <cctype>
#include <charconv>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flageasy/flageasy.hpp"

#if defined(__has_include)
#  if __has_include(<version>)
#    include <version>
#  endif
#endif

// Shortest-round-trip float formatting when <charconv> supports it.
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#  define FLAGEASY_HAS_TO_CHARS 1
#else
#  define FLAGEASY_HAS_TO_CHARS 0
#endif

namespace flageasy {

// ---------------------------------------------------------------------------
// Flag::Impl — definition data for one flag / option / positional.
// ---------------------------------------------------------------------------
struct Flag::Impl {
    std::string name;
    char short_name = '\0';
    ValueType type = ValueType::String;
    bool list = false;        // repeatable, holds N values
    bool positional = false;
    bool rest = false;        // greedy positional
    bool required = false;
    bool hidden = false;

    bool has_default = false;
    std::vector<std::string> defaults;

    bool has_range = false;
    double range_lo = 0.0;
    double range_hi = 0.0;

    std::vector<std::string> choices;
    std::function<bool(const std::string&, std::string&)> validator;

    std::string help;
    std::string metavar;
};

// ---------------------------------------------------------------------------
// Parser::Impl
// ---------------------------------------------------------------------------
struct Parser::Impl {
    std::string program;      // display name (basename of what user passed)
    std::string description;
    std::string version_text;
    std::string epilog;
    bool allow_unknown = false;
    bool has_version = false;

    std::vector<std::unique_ptr<Flag>> flags;  // definition order
    std::unordered_map<std::string, Flag*> by_name;
    std::unordered_map<char, Flag*> by_short;
    std::vector<Flag*> positionals;            // definition order
    Flag* rest_flag = nullptr;
    Flag* help_flag = nullptr;
    Flag* version_flag = nullptr;

    Flag* find_long(const std::string& name) const {
        auto it = by_name.find(name);
        return it == by_name.end() ? nullptr : it->second;
    }
    Flag* find_short(char c) const {
        auto it = by_short.find(c);
        return it == by_short.end() ? nullptr : it->second;
    }
};

// ---------------------------------------------------------------------------
// ParseResult::Impl
// ---------------------------------------------------------------------------
struct ParseResult::Impl {
    bool ok = true;
    ParseError error;
    std::vector<ParsedFlag> flags;      // one entry per defined flag
    std::vector<std::string> positionals;
    std::vector<std::string> unknown;
    std::unordered_map<std::string, std::size_t> index;  // name -> flags[]
    bool help = false;
    bool version = false;
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace internal {

/// Friend of Flag; lets internal helpers read Flag::Impl without naming the
/// private nested type.
struct FlagAccess {
    static const Flag::Impl& impl(const Flag& flag) { return *flag.m_impl; }
};

inline std::string to_lower(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text)
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

/// Reduce "a/b/app" or "C:\\bin\\app.exe" to "app" (keeps ".exe").
inline std::string basename_of(std::string_view path) {
    std::size_t slash = path.find_last_of("/\\");
    return std::string(path.substr(slash == std::string_view::npos ? 0 : slash + 1));
}

/// True when the name is non-empty, starts alphanumeric, and contains only
/// alphanumerics, '-', '_', or '.'.
inline bool is_valid_flag_name(std::string_view name) {
    if (name.empty()) return false;
    char first = name.front();
    if (!std::isalnum(static_cast<unsigned char>(first))) return false;
    for (char c : name) {
        unsigned char u = static_cast<unsigned char>(c);
        if (!std::isalnum(u) && c != '-' && c != '_' && c != '.') return false;
    }
    return true;
}

/// Case-insensitive equality. ASCII only.
inline bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

/// Shortest round-trip representation of a double ("1", "1.5", "1e+20").
inline std::string format_number(double value) {
#if FLAGEASY_HAS_TO_CHARS
    char buffer[64];
    const std::to_chars_result result =
        std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec == std::errc()) return std::string(buffer, result.ptr);
#endif
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.15g", value);
    return std::string(buffer);
}

}  // namespace internal
}  // namespace flageasy

#endif  // FLAGEASY_INTERNAL_HPP
