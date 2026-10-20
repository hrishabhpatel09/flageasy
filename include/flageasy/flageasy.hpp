// flageasy — a small, robust command-line flag parsing library for C++17.
//
// Define flags with a fluent API, hand it argc/argv, and get back a
// ParseResult that enumerates every flag with its effective values.
//
//   flageasy::Parser cli("myapp", "Does useful things.");
//   cli.add_switch("verbose", 'v', "Enable verbose output");
//   cli.add_int("count", 'c', "Repeat count").default_value(1).range(1, 100);
//   cli.add_string("name", 'n', "Your name").required();
//
//   auto r = cli.parse(argc, argv);
//   if (r.help_requested()) { std::cout << cli.help(); return 0; }
//   if (!r) { std::cerr << r.error_message() << "\n" << cli.usage() << "\n"; return 2; }
//
//   int count = r.as<int>("count");
//
// parse() never throws for user input problems; it reports them through
// ParseResult. Definition-time misuse (duplicate names, bad identifiers)
// throws std::invalid_argument.

#ifndef FLAGEASY_FLAGEASY_HPP
#define FLAGEASY_FLAGEASY_HPP

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// Export / visibility macro
// ---------------------------------------------------------------------------
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(FLAGEASY_BUILDING)
#    define FLAGEASY_API __declspec(dllexport)
#  else
#    define FLAGEASY_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define FLAGEASY_API __attribute__((visibility("default")))
#else
#  define FLAGEASY_API
#endif

namespace flageasy {

// ---------------------------------------------------------------------------
// Library version
// ---------------------------------------------------------------------------
FLAGEASY_API const char* library_version() noexcept;  // e.g. "1.0.0"
FLAGEASY_API int library_version_major() noexcept;
FLAGEASY_API int library_version_minor() noexcept;
FLAGEASY_API int library_version_patch() noexcept;

// ---------------------------------------------------------------------------
// Basic enums / structs
// ---------------------------------------------------------------------------

/// Kind of value a flag holds.
enum class ValueType {
    Switch,   ///< boolean switch; presence means true
    Integer,  ///< signed 64-bit integer
    Double,   ///< double precision float
    String    ///< text
};

FLAGEASY_API std::string_view to_string(ValueType type) noexcept;

/// Machine-readable error category for a failed parse.
enum class ErrorCode {
    None = 0,           ///< no error
    UnknownFlag,        ///< unrecognized --flag or -f
    MissingValue,       ///< an option required a value that was not given
    InvalidValue,       ///< value failed type/range/choice/validator checks
    MissingRequired,    ///< a required flag or positional was not provided
    UnexpectedArgument  ///< extra positional argument with no slot for it
};

FLAGEASY_API std::string_view to_string(ErrorCode code) noexcept;

/// Details of a failed parse. Empty when the parse succeeded.
struct ParseError {
    ErrorCode code = ErrorCode::None;
    std::string message;    ///< human readable, single line, no trailing '\n'
    std::string flag_name;  ///< long name of the flag involved ("" if n/a)
    std::string token;      ///< offending raw token ("" if n/a)
};

// ---------------------------------------------------------------------------
// detail: value conversion helpers (used by ParseResult::as<T>)
// ---------------------------------------------------------------------------
namespace detail {

FLAGEASY_API bool convert_int(const std::string& text, long long& out, std::string& error);
FLAGEASY_API bool convert_double(const std::string& text, double& out, std::string& error);
FLAGEASY_API bool convert_bool(const std::string& text, bool& out, std::string& error);

template <class T>
struct is_std_vector : std::false_type {};
template <class E, class A>
struct is_std_vector<std::vector<E, A>> : std::true_type {};

/// Convert one raw token to T; throws std::runtime_error on failure.
/// Supported scalar T: bool, std::string, integral types, floating types.
template <class T>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
convert_one(const std::string& text, const std::string& /*flag*/) {
    return text;
}

template <class T>
typename std::enable_if<std::is_integral<T>::value &&
                            !std::is_same<T, bool>::value,
                        T>::type
convert_one(const std::string& text, const std::string& flag) {
    long long value = 0;
    std::string error;
    if (!convert_int(text, value, error))
        throw std::runtime_error(error + " for flag '" + flag + "'");
    if (value < static_cast<long long>((std::numeric_limits<T>::min)()) ||
        value > static_cast<long long>((std::numeric_limits<T>::max)()))
        throw std::runtime_error("value '" + text + "' out of range for flag '" +
                                 flag + "'");
    return static_cast<T>(value);
}

template <class T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
convert_one(const std::string& text, const std::string& flag) {
    double value = 0.0;
    std::string error;
    if (!convert_double(text, value, error))
        throw std::runtime_error(error + " for flag '" + flag + "'");
    return static_cast<T>(value);
}

template <class T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type
convert_one(const std::string& text, const std::string& flag) {
    bool value = false;
    std::string error;
    if (!convert_bool(text, value, error))
        throw std::runtime_error(error + " for flag '" + flag + "'");
    return value;
}

}  // namespace detail

class Parser;
class ParseResult;

namespace internal {
struct FlagAccess;  // implementation detail (see src/internal.hpp)
}  // namespace internal

// ---------------------------------------------------------------------------
// Flag: the definition of a single switch, option, or positional argument.
// Flags are created through Parser::add_* and live inside the Parser; the
// Parser hands out references for fluent configuration.
// ---------------------------------------------------------------------------
class FLAGEASY_API Flag {
public:
    Flag(const Flag&) = delete;
    Flag& operator=(const Flag&) = delete;
    ~Flag();

    // --- introspection -----------------------------------------------------
    const std::string& name() const noexcept;
    char short_name() const noexcept;
    ValueType type() const noexcept;
    bool is_list() const noexcept;        ///< repeatable flag (may hold N values)
    bool is_positional() const noexcept;
    bool is_rest() const noexcept;        ///< positional that collects the rest
    bool is_required() const noexcept;
    bool is_hidden() const noexcept;
    bool takes_value() const noexcept;    ///< false only for switches
    const std::string& help_text() const noexcept;

    // --- fluent configuration (each returns *this) --------------------------

    /// Help text shown in the generated help output.
    Flag& help(std::string text);

    /// Custom value placeholder in help output (default derives from name).
    Flag& metavar(std::string text);

    /// Parse fails with MissingRequired if the flag is not provided.
    Flag& required(bool on = true);

    /// Omit the flag from generated usage/help output.
    Flag& hidden(bool on = true);

    /// Value used when the flag is not provided on the command line.
    /// The value is still validated (type, range, choices, validator).
    Flag& default_value(bool value);
    Flag& default_value(const char* value);
    Flag& default_value(std::string value);
    template <class T, typename std::enable_if<std::is_integral<T>::value &&
                                                   !std::is_same<T, bool>::value,
                                               int>::type = 0>
    Flag& default_value(T value) {
        return default_value_signed(static_cast<long long>(value));
    }
    template <class T, typename std::enable_if<std::is_floating_point<T>::value,
                                               int>::type = 0>
    Flag& default_value(T value) {
        return default_value_real(static_cast<double>(value));
    }

    /// Default values for list flags.
    Flag& default_values(std::vector<std::string> values);

    /// Restrict numeric values to [low, high] (inclusive).
    Flag& range(double low, double high);

    /// Restrict the raw token to one of the given strings.
    Flag& choices(std::vector<std::string> allowed);

    /// Custom check; return false and set `error` to reject the value.
    Flag& validate(std::function<bool(const std::string&, std::string&)> fn);

private:
    friend class Parser;
    friend struct internal::FlagAccess;
    struct Impl;
    explicit Flag(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;

    Flag& default_value_signed(long long value);
    Flag& default_value_real(double value);
};

// ---------------------------------------------------------------------------
// ParsedFlag: one entry in the parse result; a flag and its values.
// ---------------------------------------------------------------------------
struct ParsedFlag {
    std::string name;                  ///< long name, or positional name
    char short_name = '\0';
    ValueType type = ValueType::String;
    bool is_positional = false;
    bool is_set = false;               ///< true when given on the command line
    std::vector<std::string> values;   ///< effective values (defaults applied)

    /// First value, or "" when there is none.
    const std::string& value() const noexcept {
        static const std::string empty;
        return values.empty() ? empty : values.front();
    }
};

// ---------------------------------------------------------------------------
// ParseResult: the outcome of Parser::parse.
// ---------------------------------------------------------------------------
class FLAGEASY_API ParseResult {
public:
    ParseResult(const ParseResult& other);
    ParseResult& operator=(const ParseResult& other);
    ParseResult(ParseResult&& other) noexcept;
    ParseResult& operator=(ParseResult&& other) noexcept;
    ~ParseResult();

    // --- status -------------------------------------------------------------
    bool ok() const noexcept;
    explicit operator bool() const noexcept { return ok(); }
    const ParseError& error() const noexcept;
    const std::string& error_message() const noexcept;

    /// The user passed the built-in --help/-h flag.
    bool help_requested() const noexcept;
    /// The user passed the built-in --version/-V flag (see Parser::version).
    bool version_requested() const noexcept;

    // --- raw access ---------------------------------------------------------
    /// Is a flag (or positional) with this name defined?
    bool has(const std::string& name) const noexcept;
    /// Was the flag explicitly provided on the command line?
    bool is_set(const std::string& name) const noexcept;
    /// Effective raw values (defaults applied); throws std::out_of_range for
    /// an unknown name.
    const std::vector<std::string>& raw(const std::string& name) const;
    /// First effective raw value, if any.
    std::optional<std::string> value(const std::string& name) const;

    // --- typed access ---------------------------------------------------------
    /// Convert to T. Supported T: bool, std::string, integral and floating
    /// types, and std::vector<> of those. Throws std::runtime_error when the
    /// flag is unknown, has no value, holds multiple values (for scalar T),
    /// or the token cannot be converted.
    template <class T>
    T as(const std::string& name) const {
        const std::vector<std::string>& vals = raw(name);
        const std::string label = "'" + name + "'";
        if constexpr (detail::is_std_vector<T>::value) {
            using Element = typename T::value_type;
            T out;
            out.reserve(vals.size());
            for (const std::string& v : vals)
                out.push_back(detail::convert_one<Element>(v, label));
            return out;
        } else if constexpr (std::is_same<T, bool>::value) {
            if (vals.empty()) return false;  // unset switch
            return detail::convert_one<bool>(vals.front(), label);
        } else {
            if (vals.empty())
                throw std::runtime_error("flag " + label + " has no value");
            if (vals.size() > 1)
                throw std::runtime_error("flag " + label +
                                         " has multiple values; use "
                                         "std::vector<...> to retrieve them");
            return detail::convert_one<T>(vals.front(), label);
        }
    }

    /// Like as<T>, but returns std::nullopt instead of throwing.
    template <class T>
    std::optional<T> try_as(const std::string& name) const {
        if (!has(name)) return std::nullopt;
        try {
            return as<T>(name);
        } catch (...) {
            return std::nullopt;
        }
    }

    /// Like as<T>, but falls back to `fallback` instead of throwing.
    template <class T>
    T as_or(const std::string& name, T fallback) const {
        std::optional<T> v = try_as<T>(name);
        return v ? *v : std::move(fallback);
    }

    // --- enumeration ----------------------------------------------------------
    /// Every defined flag/positional in definition order, with effective
    /// values and is_set markers. Includes the built-in help/version flags.
    const std::vector<ParsedFlag>& flags() const noexcept;
    /// Positional values in the order they appeared on the command line.
    const std::vector<std::string>& positionals() const noexcept;
    /// Arguments captured because Parser::allow_unknown(true) was set.
    const std::vector<std::string>& unknown() const noexcept;
    /// Names of all defined flags/positionals.
    std::vector<std::string> names() const;

    /// "count=3, verbose=true, name=<unset>" style summary.
    std::string to_string() const;

private:
    friend class Parser;
    ParseResult();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
class FLAGEASY_API Parser {
public:
    /// `program_name` is used in usage/help output; a path is reduced to its
    /// basename. `description` appears at the top of help().
    explicit Parser(std::string program_name, std::string description = {});
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;
    ~Parser();

    // --- flag definition (returns a stable reference to configure) -----------

    /// Boolean switch: presence means true (--verbose), --verbose=false turns
    /// it off. A short name of '\0' means none.
    Flag& add_switch(std::string name, char short_name = '\0', std::string help = {});
    Flag& add_string(std::string name, char short_name = '\0', std::string help = {});
    Flag& add_int(std::string name, char short_name = '\0', std::string help = {});
    Flag& add_double(std::string name, char short_name = '\0', std::string help = {});

    /// Repeatable flags: each occurrence appends one value.
    Flag& add_string_list(std::string name, char short_name = '\0', std::string help = {});
    Flag& add_int_list(std::string name, char short_name = '\0', std::string help = {});
    Flag& add_double_list(std::string name, char short_name = '\0', std::string help = {});

    /// Positional argument. Non-rest positionals are required by default;
    /// call .required(false) to make one optional. Values are always strings
    /// but as<int>() etc. converts them.
    Flag& add_positional(std::string name, std::string help = {});
    /// Positional that collects every remaining argument (add at most one,
    /// after all other positionals). Never required.
    Flag& add_positional_rest(std::string name, std::string help = {});

    // --- parser configuration -------------------------------------------------
    /// Set a version string; adds a --version/-V switch that triggers
    /// ParseResult::version_requested().
    Parser& version(std::string text);
    Parser& description(std::string text);
    /// Appended verbatim at the end of help().
    Parser& epilog(std::string text);
    /// Capture unrecognized flags and surplus positionals instead of failing;
    /// retrieve them via ParseResult::unknown().
    Parser& allow_unknown(bool on = true);

    // --- introspection ----------------------------------------------------------
    const std::string& program_name() const noexcept;
    const std::string& version_text() const noexcept;
    /// All defined flags (including hidden ones and built-ins), in order.
    std::vector<const Flag*> flags() const;

    // --- output -----------------------------------------------------------------
    /// One-line usage string, e.g. "Usage: app -o FILE [options] <input>".
    std::string usage() const;
    /// Full help text (description, usage, positionals, options, epilog).
    std::string help() const;

    // --- parsing ------------------------------------------------------------------
    /// Parse argv (argv[0] is treated as the program name and skipped).
    /// Never throws for bad user input; inspect the returned ParseResult.
    ParseResult parse(int argc, const char* const argv[]) const;
    /// Parse an argument list *not* including the program name.
    ParseResult parse(const std::vector<std::string>& args) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    Flag& add_flag(std::string name, char short_name, std::string help,
                   ValueType type, bool list, bool positional, bool rest);
};

}  // namespace flageasy

#endif  // FLAGEASY_FLAGEASY_HPP
