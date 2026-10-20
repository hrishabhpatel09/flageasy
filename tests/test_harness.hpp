// flageasy — shared test harness: macros, helpers, test declarations.

#ifndef FLAGEASY_TEST_HARNESS_HPP
#define FLAGEASY_TEST_HARNESS_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <flageasy/flageasy.hpp>

extern int g_checks;
extern int g_failures;

// --- printable values for assertion messages --------------------------------

template <class T>
std::string show(const T& value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

template <class T>
std::string show(const std::vector<T>& values) {
    std::string out = "[";
    for (std::size_t k = 0; k < values.size(); ++k) {
        if (k != 0) out += ", ";
        out += show(values[k]);
    }
    return out + "]";
}

inline std::string show(const std::vector<bool>& values) {
    std::string out = "[";
    for (std::size_t k = 0; k < values.size(); ++k) {
        if (k != 0) out += ", ";
        out += values[k] ? "true" : "false";
    }
    return out + "]";
}

inline std::string show(flageasy::ValueType type) {
    return std::string(flageasy::to_string(type));
}

inline std::string show(flageasy::ErrorCode code) {
    return std::string(flageasy::to_string(code));
}

// --- helpers -----------------------------------------------------------------

inline flageasy::ParseResult run(const flageasy::Parser& cli,
                                 std::vector<std::string> args) {
    return cli.parse(args);
}

inline const flageasy::ParsedFlag* find_flag(const flageasy::ParseResult& result,
                                             const std::string& name) {
    for (const flageasy::ParsedFlag& flag : result.flags())
        if (flag.name == name) return &flag;
    return nullptr;
}

// --- macros -------------------------------------------------------------------

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": "      \
                      << #cond << "\n";                                      \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b)                                                       \
    do {                                                                     \
        ++g_checks;                                                          \
        auto&& check_a = (a);                                                \
        auto&& check_b = (b);                                                \
        if (!(check_a == check_b)) {                                         \
            ++g_failures;                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << ": "      \
                      << #a << " == " << #b << " (" << show(check_a)         \
                      << " vs " << show(check_b) << ")\n";                   \
        }                                                                    \
    } while (0)

#define CHECK_THROWS(expr)                                                   \
    do {                                                                     \
        ++g_checks;                                                          \
        bool threw = false;                                                  \
        try {                                                                \
            (void)(expr);                                                    \
        } catch (...) {                                                      \
            threw = true;                                                    \
        }                                                                    \
        if (!threw) {                                                        \
            ++g_failures;                                                    \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__              \
                      << ": expected exception from: " #expr << "\n";        \
        }                                                                    \
    } while (0)

// --- test declarations ---------------------------------------------------------

void test_switches();
void test_string_option_forms();
void test_short_clusters();
void test_positionals();
void test_optional_positional();
void test_double_dash();
void test_empty_values();

void test_int_option();
void test_double_option();
void test_negative_numbers();
void test_list_options();
void test_defaults();
void test_unknown_flags();
void test_allow_unknown();
void test_parse_argv();
void test_result_copy();

void test_missing_value();
void test_required();
void test_choices();
void test_range();
void test_custom_validator();
void test_definition_errors();
void test_help_and_version();
void test_usage();
void test_enumeration();
void test_raw_access();
void test_conversions();
void test_misc_functions();

#endif  // FLAGEASY_TEST_HARNESS_HPP
