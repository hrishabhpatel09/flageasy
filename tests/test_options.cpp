// flageasy — tests: numeric options, lists, defaults, unknown-flag handling.

#include <cstdint>

#include "test_harness.hpp"

void test_int_option() {
    flageasy::Parser cli("app");
    cli.add_int("count", 'c', "Count").default_value(1).range(1, 100);

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK(!r.is_set("count"));
    CHECK_EQ(r.as<int>("count"), 1);
    CHECK_EQ(r.raw("count"), std::vector<std::string>{"1"});

    r = run(cli, {"--count", "42"});
    CHECK(r.ok());
    CHECK(r.is_set("count"));
    CHECK_EQ(r.as<int>("count"), 42);
    CHECK_EQ(r.as<long long>("count"), 42LL);

    r = run(cli, {"-c", "5"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("count"), 5);

    r = run(cli, {"--count=-7"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("range") != std::string::npos);

    r = run(cli, {"-c", "0"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--count", "abc"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK_EQ(r.error().flag_name, std::string("count"));

    r = run(cli, {"--count", "0x10"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--count", "99999999999999999999"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--count", "5", "--count", "6"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("count"), 6);
    CHECK_EQ(r.raw("count").size(), std::size_t(1));

    // narrowing conversions through as<T>
    flageasy::Parser wide("app");
    wide.add_int("value", 'V', "Value");
    r = run(wide, {"--value", "300"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("value"), 300);
    CHECK_EQ(r.as<long long>("value"), 300LL);
    CHECK(!r.try_as<std::int8_t>("value").has_value());   // 300 > 127
    CHECK(r.as_or<std::int8_t>("value", std::int8_t(9)) == std::int8_t(9));
}

void test_double_option() {
    flageasy::Parser cli("app");
    cli.add_double("ratio", 'r', "Ratio").default_value(0.5).range(0.1, 10.0);
    cli.add_double("scale", 's', "Scale");

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK(r.as<double>("ratio") == 0.5);
    CHECK_EQ(r.raw("ratio"), std::vector<std::string>{"0.5"});

    r = run(cli, {"--ratio", "2.5"});
    CHECK(r.ok());
    CHECK(r.as<double>("ratio") == 2.5);

    r = run(cli, {"--ratio=1.5e1"});
    CHECK(!r.ok());  // 15 is outside [0.1, 10]
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--ratio", "0.05"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--ratio", "abc"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    r = run(cli, {"--ratio", "0.1"});
    CHECK(r.ok());  // boundary is inclusive
    r = run(cli, {"--ratio", "10"});
    CHECK(r.ok());

    r = run(cli, {"--scale", "-1.25"});
    CHECK(r.ok());
    CHECK(r.as<double>("scale") == -1.25);

    r = run(cli, {});
    CHECK(!r.try_as<double>("scale").has_value());
    CHECK_THROWS(r.as<double>("scale"));
}

void test_negative_numbers() {
    flageasy::Parser cli("app");
    cli.add_int("num", 'n', "Number");
    cli.add_string("name", 'N', "Name");

    auto r = run(cli, {"--num", "-5"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("num"), -5);

    r = run(cli, {"-n", "-5"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("num"), -5);

    r = run(cli, {"--name", "-5"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("-5"));

    // a lone dash is a value (stdin convention)
    r = run(cli, {"--name", "-"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("-"));

    // values that look like options require the = form
    r = run(cli, {"--name", "-x"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingValue);

    r = run(cli, {"--name=-x"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("-x"));
}

void test_list_options() {
    flageasy::Parser cli("app");
    cli.add_string_list("include", 'I', "Include path");
    cli.add_int_list("port", 'p', "Port");
    cli.add_double_list("weight", 'w', "Weight");

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK(r.raw("include").empty());
    CHECK_EQ(r.as<std::vector<std::string>>("include"),
             std::vector<std::string>{});

    r = run(cli, {"-I", "a", "-I", "b"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::vector<std::string>>("include"),
             (std::vector<std::string>{"a", "b"}));

    r = run(cli, {"--include=a"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::vector<std::string>>("include"),
             std::vector<std::string>{"a"});

    r = run(cli, {"-I", "a", "--include=b", "-Ic"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::vector<std::string>>("include"),
             (std::vector<std::string>{"a", "b", "c"}));

    r = run(cli, {"-p", "80", "-p", "8080"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::vector<int>>("port"), (std::vector<int>{80, 8080}));
    CHECK_EQ(r.raw("port"), (std::vector<std::string>{"80", "8080"}));

    r = run(cli, {"-w", "1.5", "-w", "2.5"});
    CHECK(r.ok());
    CHECK(r.as<std::vector<double>>("weight") ==
          (std::vector<double>{1.5, 2.5}));

    r = run(cli, {"-p", "x"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    // scalar retrieval of a multi-valued flag fails, vector retrieval works
    r = run(cli, {"-I", "a", "-I", "b"});
    CHECK(!r.try_as<std::string>("include").has_value());
    CHECK_EQ(r.as_or<std::string>("include", std::string("fallback")),
             std::string("fallback"));

    r = run(cli, {"-I", "only"});
    CHECK_EQ(r.as<std::string>("include"), std::string("only"));
}

void test_defaults() {
    flageasy::Parser cli("app");
    cli.add_int("count", 'c', "Count").default_value(3);
    cli.add_string_list("tag", 't', "Tag").default_values({"x", "y"});
    cli.add_switch("verbose", 'v', "Verbose").default_value(true);

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("count"), 3);
    CHECK(!r.is_set("count"));
    CHECK_EQ(r.as<std::vector<std::string>>("tag"),
             (std::vector<std::string>{"x", "y"}));
    CHECK(!r.is_set("tag"));
    CHECK(r.as<bool>("verbose"));
    CHECK(!r.is_set("verbose"));

    // explicit values replace defaults
    r = run(cli, {"--count", "9", "--tag", "z"});
    CHECK(r.ok());
    CHECK(r.is_set("count"));
    CHECK_EQ(r.as<int>("count"), 9);
    CHECK_EQ(r.as<std::vector<std::string>>("tag"),
             std::vector<std::string>{"z"});

    // invalid defaults are reported at parse time
    flageasy::Parser broken("app");
    broken.add_int("count", 'c', "Count").default_value("abc");
    r = run(broken, {});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("invalid default") != std::string::npos);
}

void test_unknown_flags() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");

    auto r = run(cli, {"--verbos"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnknownFlag);
    CHECK(r.error_message().find("did you mean '--verbose'") != std::string::npos);
    CHECK_EQ(r.error().flag_name, std::string("verbos"));
    CHECK_EQ(r.error().token, std::string("--verbos"));

    r = run(cli, {"-z"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnknownFlag);
    CHECK(r.error_message().find("'-z'") != std::string::npos);

    // case-insensitive short suggestion (-H -> -h)
    r = run(cli, {"-H"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnknownFlag);
    CHECK(r.error_message().find("'-h'") != std::string::npos);

    // surplus argument with no positional slot
    r = run(cli, {"extra"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnexpectedArgument);
}

void test_allow_unknown() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_positional_rest("rest", "Rest");
    cli.allow_unknown(true);

    auto r = run(cli, {"--wat", "x", "-z", "--foo=1", "-v"});
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));
    CHECK_EQ(r.unknown(),
             (std::vector<std::string>{"--wat", "-z", "--foo=1"}));
    CHECK_EQ(r.as<std::vector<std::string>>("rest"),
             std::vector<std::string>{"x"});

    // surplus positionals are captured too
    flageasy::Parser limited("app");
    limited.add_positional("one", "One");
    limited.allow_unknown(true);
    r = run(limited, {"a", "b"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("one"), std::string("a"));
    CHECK_EQ(r.unknown(), std::vector<std::string>{"b"});
}

void test_parse_argv() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_int("count", 'c', "Count");

    const char* argv[] = {"app", "--verbose", "--count", "5"};
    auto r = cli.parse(4, argv);
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));
    CHECK_EQ(r.as<int>("count"), 5);

    const char* only_program[] = {"app"};
    r = cli.parse(1, only_program);
    CHECK(r.ok());
    CHECK(!r.is_set("verbose"));
}

void test_result_copy() {
    flageasy::Parser cli("app");
    cli.add_int("count", 'c', "Count");
    cli.add_string("name", 'n', "Name");

    auto original = run(cli, {"--count", "7", "--name", "Alice"});
    CHECK(original.ok());

    flageasy::ParseResult copy = original;  // deep copy
    CHECK(copy.ok());
    CHECK_EQ(copy.as<int>("count"), 7);
    CHECK_EQ(copy.as<std::string>("name"), std::string("Alice"));

    flageasy::ParseResult copy2 = copy;
    copy2 = original;  // copy assignment
    CHECK(copy2.ok());
    CHECK_EQ(copy2.as<int>("count"), 7);

    flageasy::ParseResult moved(std::move(original));
    CHECK(moved.ok());
    CHECK_EQ(moved.as<int>("count"), 7);
}
