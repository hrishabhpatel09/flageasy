// flageasy — demo: a small "imgtool" that shows the intended usage.
//
// Build with:
//   cmake -S . -B build && cmake --build build
// then run:
//   ./build/examples/flageasy_demo --help

#include <iostream>
#include <string>

#include <flageasy/flageasy.hpp>

int main(int argc, char** argv) {
    flageasy::Parser cli("imgtool", "Convert images between formats.");
    cli.version("imgtool 1.0.0");
    cli.epilog("Report bugs at https://example.com/imgtool/issues");

    // Optional flags.
    cli.add_switch("verbose", 'v', "Enable verbose output");
    auto& output = cli.add_string("output", 'o', "Output file").required();
    cli.add_int("quality", 'q', "JPEG quality")
        .default_value(85)
        .range(1, 100);
    cli.add_string("format", 'f', "Output format")
        .choices({"png", "jpeg", "webp"})
        .default_value("png");
    cli.add_double("scale", 's', "Scale factor")
        .default_value(1.0)
        .range(0.1, 10.0);
    cli.add_string_list("tag", 't', "Metadata tag (repeatable)");

    // Positionals.
    cli.add_positional("input", "Input file");
    cli.add_positional_rest("extra", "Additional input files");

    flageasy::ParseResult result = cli.parse(argc, argv);

    if (result.help_requested()) {
        std::cout << cli.help();
        return 0;
    }
    if (result.version_requested()) {
        std::cout << cli.version_text() << "\n";
        return 0;
    }
    if (!result.ok()) {
        std::cerr << "error: " << result.error_message() << "\n\n"
                  << cli.usage() << "\n";
        return 2;
    }

    // Typed access.
    std::cout << "input   : " << result.as<std::string>("input") << "\n";
    std::cout << "output  : " << result.as<std::string>("output") << "\n";
    std::cout << "format  : " << result.as<std::string>("format") << "\n";
    std::cout << "quality : " << result.as<int>("quality") << "\n";
    std::cout << "scale   : " << result.as<double>("scale") << "\n";
    std::cout << "verbose : " << (result.as<bool>("verbose") ? "yes" : "no")
              << "\n";

    std::cout << "extra   : ";
    for (const std::string& file : result.as<std::vector<std::string>>("extra"))
        std::cout << file << " ";
    std::cout << "\n";

    std::cout << "tags    : ";
    for (const std::string& tag : result.as<std::vector<std::string>>("tag"))
        std::cout << tag << " ";
    std::cout << "\n";

    // Every flag and its effective value, regardless of type.
    std::cout << "\nall flags: " << result.to_string() << "\n";

    (void)output;
    return 0;
}
