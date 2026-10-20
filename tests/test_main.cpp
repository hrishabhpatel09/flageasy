// flageasy — test runner main.

#include "test_harness.hpp"

int g_checks = 0;
int g_failures = 0;

int main() {
    test_switches();
    test_string_option_forms();
    test_short_clusters();
    test_positionals();
    test_optional_positional();
    test_double_dash();
    test_empty_values();

    test_int_option();
    test_double_option();
    test_negative_numbers();
    test_list_options();
    test_defaults();
    test_unknown_flags();
    test_allow_unknown();
    test_parse_argv();
    test_result_copy();

    test_missing_value();
    test_required();
    test_choices();
    test_range();
    test_custom_validator();
    test_definition_errors();
    test_help_and_version();
    test_usage();
    test_enumeration();
    test_raw_access();
    test_conversions();
    test_misc_functions();

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
