// flageasy — tests: switches, basic options, positionals.

#include "test_harness.hpp"

void test_switches() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose output");
    cli.add_switch("quiet", 'q', "Quiet output");

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK(!r.is_set("verbose"));
    CHECK(!r.as<bool>("verbose"));
    CHECK_EQ(find_flag(r, "verbose")->values, std::vector<std::string>{"false"});
    CHECK_EQ(find_flag(r, "verbose")->type, flageasy::ValueType::Switch);

    r = run(cli, {"--verbose"});
    CHECK(r.ok());
    CHECK(r.is_set("verbose"));
    CHECK(r.as<bool>("verbose"));
    CHECK(!r.as<bool>("quiet"));

    r = run(cli, {"-v"});
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));

    r = run(cli, {"-vq"});
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));
    CHECK(r.as<bool>("quiet"));

    r = run(cli, {"--verbose=false"});
    CHECK(r.ok());
    CHECK(r.is_set("verbose"));
    CHECK(!r.as<bool>("verbose"));

    r = run(cli, {"-q=false"});
    CHECK(r.ok());
    CHECK(r.is_set("quiet"));
    CHECK(!r.as<bool>("quiet"));

    r = run(cli, {"--verbose=yes"});
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));

    r = run(cli, {"--verbose=banana"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);
    CHECK(r.error_message().find("invalid boolean value") != std::string::npos);

    r = run(cli, {"--verbose="});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    // -vh: v sets verbose, h triggers help; required checks are skipped.
    r = run(cli, {"-v", "-h"});
    CHECK(r.ok());
    CHECK(r.help_requested());
    CHECK(r.as<bool>("verbose"));

    r = run(cli, {"-qh"});
    CHECK(r.ok());
    CHECK(r.help_requested());
    CHECK(r.as<bool>("quiet"));
}

void test_string_option_forms() {
    flageasy::Parser cli("app");
    cli.add_string("name", 'n', "Name").required();

    auto r = run(cli, {"--name", "Alice"});
    CHECK(r.ok());
    CHECK(r.is_set("name"));
    CHECK_EQ(r.as<std::string>("name"), std::string("Alice"));
    CHECK_EQ(r.raw("name"), std::vector<std::string>{"Alice"});

    r = run(cli, {"--name=Alice"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("Alice"));

    r = run(cli, {"-n", "Alice"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("Alice"));

    r = run(cli, {"-nAlice"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("Alice"));

    r = run(cli, {"-n=Alice"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("Alice"));

    r = run(cli, {});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);

    // duplicate non-list flag: last occurrence wins
    r = run(cli, {"--name", "Alice", "--name", "Bob"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("name"), std::string("Bob"));
    CHECK_EQ(r.raw("name").size(), std::size_t(1));
}

void test_short_clusters() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_string("output", 'o', "Output file");
    cli.add_int("level", 'l', "Level");

    auto r = run(cli, {"-oout.txt"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("output"), std::string("out.txt"));

    r = run(cli, {"-o=out.txt"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("output"), std::string("out.txt"));

    r = run(cli, {"-o", "out.txt"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("output"), std::string("out.txt"));

    r = run(cli, {"-vo", "out.txt"});
    CHECK(r.ok());
    CHECK(r.as<bool>("verbose"));
    CHECK_EQ(r.as<std::string>("output"), std::string("out.txt"));

    r = run(cli, {"-l7"});
    CHECK(r.ok());
    CHECK_EQ(r.as<int>("level"), 7);

    // value-taking short consumes the rest of the cluster
    r = run(cli, {"-lx"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::InvalidValue);

    // unknown short inside a cluster
    r = run(cli, {"-vz"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnknownFlag);
    CHECK(r.error_message().find("'-z'") != std::string::npos);
    CHECK_EQ(r.error().token, std::string("-vz"));
}

void test_positionals() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_positional("input", "Input file");
    cli.add_positional_rest("extra", "Extra files");

    auto r = run(cli, {"file.txt"});
    CHECK(r.ok());
    CHECK(r.is_set("input"));
    CHECK_EQ(r.as<std::string>("input"), std::string("file.txt"));
    CHECK(!r.is_set("extra"));
    CHECK_EQ(r.as<std::vector<std::string>>("extra"), std::vector<std::string>{});
    CHECK_EQ(r.positionals(), std::vector<std::string>{"file.txt"});

    r = run(cli, {"a", "b", "c"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("input"), std::string("a"));
    CHECK_EQ(r.as<std::vector<std::string>>("extra"),
             (std::vector<std::string>{"b", "c"}));
    CHECK_EQ(r.positionals(), (std::vector<std::string>{"a", "b", "c"}));

    // flags can be interleaved with positionals
    r = run(cli, {"file.txt", "-v"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("input"), std::string("file.txt"));
    CHECK(r.as<bool>("verbose"));

    // all positionals are required by default
    r = run(cli, {});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);
    CHECK(r.error_message().find("input") != std::string::npos);

    // surplus positional without a rest slot
    flageasy::Parser strict("app");
    strict.add_positional("input", "Input file");
    r = run(strict, {"a", "b"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::UnexpectedArgument);
    CHECK_EQ(r.error().token, std::string("b"));
}

void test_optional_positional() {
    flageasy::Parser cli("app");
    cli.add_positional("input", "Input file").required(false);

    auto r = run(cli, {});
    CHECK(r.ok());
    CHECK(!r.is_set("input"));
    CHECK(!r.try_as<std::string>("input").has_value());
    CHECK(r.positionals().empty());

    r = run(cli, {"x"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("input"), std::string("x"));
}

void test_double_dash() {
    flageasy::Parser cli("app");
    cli.add_switch("verbose", 'v', "Verbose");
    cli.add_positional("first", "First");
    cli.add_positional_rest("rest", "Rest");

    auto r = run(cli, {"--", "--verbose"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("first"), std::string("--verbose"));
    CHECK(!r.is_set("verbose"));  // after -- everything is positional

    r = run(cli, {"a", "--", "-v"});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("first"), std::string("a"));
    CHECK_EQ(r.as<std::vector<std::string>>("rest"),
             (std::vector<std::string>{"-v"}));
    CHECK(!r.is_set("verbose"));

    r = run(cli, {"--"});
    CHECK(!r.ok());
    CHECK_EQ(r.error().code, flageasy::ErrorCode::MissingRequired);
    CHECK(r.error_message().find("first") != std::string::npos);
}

void test_empty_values() {
    flageasy::Parser cli("app");
    cli.add_string("tag", 't', "Tag");

    auto r = run(cli, {"--tag="});
    CHECK(r.ok());
    CHECK(r.is_set("tag"));
    CHECK_EQ(r.as<std::string>("tag"), std::string(""));

    r = run(cli, {"--tag", ""});
    CHECK(r.ok());
    CHECK_EQ(r.as<std::string>("tag"), std::string(""));
}
