// flageasy — tests: validation, required flags, help output, conversions.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "test_harness.hpp"

void test_missing_value() {
    flageasy::Parser cli("app");
    cli.add_string("name", 'n', "Name");
    cli.add_switch("verbose", 'v', "Verbose");

    auto r = run(cli, {"--name"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingValue);
    CHECK(r.error_message().find("requires a value") != std::string::npos);

    r = run(cli, {"-n"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingValue);

    r = run(cli, {"--name", "--verbose"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingValue);
    CHECK(r.error_message().find("--name=VALUE") != std::string::npos);

    r = run(cli, {"--name", "-v"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingValue);

    // a lone dash is a value
    r = run(cli, {"--name", "-"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("-"));
}

void test_required() {
    flageasy::Parser cli("app");
    cli.add_int("count", 'c', "Count").required();
    cli.add_string("output", 'o', "Output").required();
    cli.add_positional("input", "Input");

    auto r = run(cli, {});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);
    CHECK_EQ(r.error().flag_name, std::string("count"));
    CHECK(r.error_message().find("--count") != std::string::npos);

    r = run(cli, {"-c", "1"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);
    CHECK_EQ(r.error().flag_name, std::string("output"));

    r = run(cli, {"-c", "1", "-o", "x"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);
    CHECK_EQ(r.error().flag_name, std::string("input"));

    r = run(cli, {"-c", "1", "-o", "x", "in.txt"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("count"), 1);
    CHECK_EQ(r.as<std::string>("output"), std::string("x"));
    CHECK_EQ(r.as<std::string>("input"), std::string("in.txt"));

    // --help wins over missing required flags
    r = run(cli, {"--help"});
    CHECK(r.ok());
    CHECK(r.help_requested());
}

void test_choices() {
    flageasy::Parser cli("app");
    cli.add_string("format", 'f', "Format")
        .choices({"png", "jpeg", "webp"})
        .default_value("png");

    auto r = run(cli, {"--format", "jpeg"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("format"), std::string("jpeg"));

    r = run(cli, {"--format", "gif"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("not one of: png, jpeg, webp") !=
          std::string::npos);

    r = run(cli, {});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("format"), std::string("png"));

    cli.add_int("level", 'l', "Level").choices({"1", "2"});
    r = run(cli, {"--level", "2"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("level"), 2);

    r = run(cli, {"--level", "3"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
}

void test_range() {
    flageasy::Parser cli("app");
    cli.add_int("x", 'x', "X").range(1, 5);
    cli.add_double("r", 'r', "R").range(-1.0, 1.0);

    auto r = run(cli, {"-x", "1"});
    CHECK(r.ok());
    r = run(cli, {"-x", "5"});
    CHECK(r.ok());
    r = run(cli, {"-x", "6"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("range") != std::string::npos);

    r = run(cli, {"-r", "-1"});
    CHECK(r.ok());
    r = run(cli, {"-r", "-1.5"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    // default value outside the range is reported as a definition problem
    flageasy::Parser broken("app");
    broken.add_int("x", 'x', "X").range(1, 5).default_value(9);
    r = run(broken, {});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("invalid default") != std::string::npos);
}

void test_custom_validator() {
    flageasy::Parser cli("app");
    cli.add_int("port", 'p', "Port")
        .validate([](const std::string& value, std::string& error) {
            const int port = std::stoi(value);
            if (port % 2 != 0) {
                error = "port must be even";
                return false;
            }
            return true;
        });

    auto r = run(cli, {"-p", "4"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("port"), 4);

    r = run(cli, {"-p", "3"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("port must be even") != std::string::npos);
}

void test_definition_errors() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v');
    CHECK_THROWS(cli.add_switch("verbose"));       // duplicate long name
    CHECK_THROWS(cli.add_switch("other", 'v'));    // duplicate short name
    CHECK_THROWS(cli.add_switch("bad name"));      // space in name
    CHECK_THROWS(cli.add_switch("bad!name"));      // invalid character
    CHECK_THROWS(cli.add_switch(""));              // empty name
    CHECK_THROWS(cli.add_switch("dash", '-'));     // reserved short
    CHECK_THROWS(cli.add_switch("eq", '='));       // reserved short

    flageasy::Parser pos("app");
    pos.add_positional("a");
    pos.add_positional_rest("rest");
    CHECK_THROWS(pos.add_positional("b"));         // positional after rest
    CHECK_THROWS(pos.add_positional_rest("more"));  // second rest

    flageasy::Parser clash("app");
    clash.add_positional("input");
    CHECK_THROWS(clash.add_switch("input"));       // clashes with positional
}

void test_help_and_version() {
    flageasy::Parser cli("app", "Does useful things.");
    cli.version("app 1.2.3");
    cli.add_switch("verbose", 'v', "Enable verbose output");
    cli.add_int("count", 'c', "Repeat count").default_value(1).range(1, 100);
    cli.add_string("output", 'o', "Output file").required();

    auto r = run(cli, {"-h"});
    CHECK(r.ok());
    CHECK(r.help_requested());
    CHECK(!r.version_requested());

    r = run(cli, {"--help"});
    CHECK(r.ok());
    CHECK(r.help_requested());

    r = run(cli, {"--version"});
    CHECK(r.ok());
    CHECK(r.version_requested());
    CHECK(!r.help_requested());

    r = run(cli, {"-V"});
    CHECK(r.ok());
    CHECK(r.version_requested());

    CHECK_EQ(cli.version_text(), std::string("app 1.2.3"));

    const std::string help = cli.help();
    CHECK(help.find("Does useful things.") != std::string::npos);
    CHECK(help.find("Usage: app") != std::string::npos);
    CHECK(help.find("--help") != std::string::npos);
    CHECK(help.find("Show this help message and exit") != std::string::npos);
    CHECK(help.find("-V, --version") != std::string::npos);
    CHECK(help.find("-v, --verbose") != std::string::npos);
    CHECK(help.find("-c, --count COUNT") != std::string::npos);
    CHECK(help.find("(default: 1; range: 1..100)") != std::string::npos);
    CHECK(help.find("(required)") != std::string::npos);
    CHECK(help.find("Positional arguments:") == std::string::npos);

    // narrow terminal: everything wraps inside COLUMNS
    setenv("COLUMNS", "50", 1);
    const std::string narrow = cli.help();
    unsetenv("COLUMNS");
    std::size_t longest = 0;
    std::istringstream stream(narrow);
    std::string line;
    while (std::getline(stream, line)) longest = std::max(longest, line.size());
    CHECK(longest <= 50);
}

void test_usage() {
    flageasy::Parser cli("/usr/local/bin/tool");
    CHECK_EQ(cli.program_name(), std::string("tool"));
    cli.add_int("count", 'c', "Count").required();
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_positional("input", "Input file");
    cli.add_positional_rest("extra", "Extra files");

    const std::string usage = cli.usage();
    CHECK_EQ(usage.rfind("Usage: tool", 0), std::size_t(0));
    CHECK(usage.find("-c COUNT") != std::string::npos);
    CHECK(usage.find("[options]") != std::string::npos);
    CHECK(usage.find("<input>") != std::string::npos);
    CHECK(usage.find("[<extra> ...]") != std::string::npos);
}

void test_enumeration() {
    flageasy::Parser cli("app");
    cli.version("app 1.0");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_int("count", 'c', "Count").default_value(2);
    cli.add_string("name", 'n', "Name");
    cli.add_positional("input", "Input");

    auto r = run(cli, {"-v", "-c", "3", "file.txt"});
    CHECK(r.ok());

    const flageasy::ParsedFlag* verbose = find_flag(r, "verbose");
    CHECK(verbose != nullptr);
    CHECK_EQ(verbose->type, flageasy::ValueType::Switch);
    CHECK(verbose->is_set);
    CHECK_EQ(verbose->value(), std::string("true"));

    const flageasy::ParsedFlag* count = find_flag(r, "count");
    CHECK(count != nullptr);
    CHECK_EQ(count->short_name, 'c');
    CHECK(count->is_set);
    CHECK_EQ(count->values, std::vector<std::string>{"3"});

    const flageasy::ParsedFlag* name = find_flag(r, "name");
    CHECK(name != nullptr);
    CHECK(!name->is_set);
    CHECK(name->values.empty());

    const flageasy::ParsedFlag* input = find_flag(r, "input");
    CHECK(input != nullptr);
    CHECK(input->is_positional);
    CHECK_EQ(input->value(), std::string("file.txt"));

    CHECK_EQ(r.positionals(), std::vector<std::string>{"file.txt"});

    const std::vector<std::string> names = r.names();
    CHECK(std::find(names.begin(), names.end(), "help") != names.end());
    CHECK(std::find(names.begin(), names.end(), "version") != names.end());
    CHECK(std::find(names.begin(), names.end(), "verbose") != names.end());
    CHECK(std::find(names.begin(), names.end(), "input") != names.end());

    const std::string summary = r.to_string();
    CHECK(summary.find("count=3") != std::string::npos);
    CHECK(summary.find("verbose=true") != std::string::npos);
    CHECK(summary.find("name=<unset>") != std::string::npos);
    CHECK(summary.find("help=false(default)") != std::string::npos);
}

void test_raw_access() {
    flageasy::Parser cli("app");
    cli.add_int("count", 'c', "Count").default_value(2);
    cli.add_string("name", 'n', "Name");

    auto r = run(cli, {"-c", "5"});
    CHECK(r.ok());

    CHECK(r.has("count"));
    CHECK(r.has("name"));
    CHECK(r.has("help"));
    CHECK(!r.has("nope"));

    CHECK(r.is_set("count"));
    CHECK(!r.is_set("name"));
    CHECK(!r.is_set("nope"));

    CHECK_EQ(r.raw("count"), std::vector<std::string>{"5"});
    CHECK_THROWS(r.raw("nope"));

    const std::optional<std::string> value = r.value("count");
    CHECK(value.has_value());
    CHECK_EQ(*value, std::string("5"));

    CHECK(!r.value("name").has_value());  // defined but unset, no default
    CHECK(!r.value("nope").has_value());  // unknown: nullopt, no throw
}

void test_conversions() {
    flageasy::Parser cli("app");
    cli.add_string("num", 'N', "Num");
    cli.add_string("word", 'w', "Word");
    cli.add_string_list("list", 'l', "List");
    cli.add_int_list("ints", 'i', "Ints");
    cli.add_switch("flag", 'f', "Flag");

    auto r = run(cli, {"--num", "12", "--word", "abc", "--list", "a",
                       "--list", "b", "--ints", "1", "--ints", "2"});
    CHECK(r.ok());

    CHECK_EQ(r.as<int>("num"), 12);
    CHECK_EQ(r.as<long long>("num"), 12LL);
    CHECK_EQ(r.as<unsigned>("num"), 12u);
    CHECK(r.as<double>("num") == 12.0);
    CHECK_EQ(r.as<std::string>("num"), std::string("12"));
    CHECK_THROWS(r.as<bool>("num"));  // "12" is not a boolean

    CHECK_THROWS(r.as<int>("word"));  // "abc" is not an integer
    CHECK(!r.try_as<int>("word").has_value());
    CHECK_EQ(r.as_or<int>("word", 99), 99);
    CHECK_EQ(r.as_or<int>("num", 99), 12);

    CHECK_EQ(r.as<std::vector<std::string>>("list"),
             (std::vector<std::string>{"a", "b"}));
    CHECK_EQ(r.as<std::vector<int>>("ints"), (std::vector<int>{1, 2}));
    CHECK_EQ(r.as<std::vector<long long>>("ints"),
             (std::vector<long long>{1, 2}));
    CHECK(r.as<std::vector<double>>("ints") ==
          (std::vector<double>{1.0, 2.0}));

    CHECK(!r.as<bool>("flag"));  // unset switch is false
    CHECK_EQ(r.as<std::string>("flag"), std::string("false"));

    CHECK(!r.try_as<int>("missing").has_value());  // unknown flag
    CHECK_THROWS(r.as<int>("missing"));

    // unsigned retrieval rejects negative values
    flageasy::Parser neg("app");
    neg.add_int("value", 'V', "Value");
    auto r2 = run(neg, {"--value", "-5"});
    CHECK(r2.ok());
    CHECK_EQ(r2.as<int>("value"), -5);
    CHECK(!r2.try_as<unsigned>("value").has_value());
}

void test_misc_functions() {
    CHECK_EQ(std::string(flageasy::to_string(flageasy::ErrorCode::UnknownFlag)),
             std::string("unknown_flag"));
    CHECK_EQ(std::string(flageasy::to_string(flageasy::ValueType::Integer)),
             std::string("int"));
    CHECK_EQ(std::string(flageasy::library_version()), std::string("1.0.0"));
    CHECK(flageasy::library_version_major() == 1);
    CHECK(flageasy::library_version_minor() == 0);
    CHECK(flageasy::library_version_patch() == 0);
}
