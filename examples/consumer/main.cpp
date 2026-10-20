// flageasy — standalone consumer example.
//
// This project is NOT part of the main build; it demonstrates how another
// project consumes the installed library via find_package(flageasy).
// See scripts/test_install.sh.

#include <iostream>
#include <string>

#include <flageasy/flageasy.hpp>

int main(int argc, char** argv) {
    flageasy::Parser cli("consumer", "Says hello.");
    cli.add_string("name", 'n', "Who to greet").default_value("world");

    flageasy::ParseResult result = cli.parse(argc, argv);
    if (result.help_requested()) {
        std::cout << cli.help();
        return 0;
    }
    if (!result.ok()) {
        std::cerr << "error: " << result.error_message() << "\n";
        return 1;
    }
    std::cout << "hello, " << result.as<std::string>("name") << "\n";
    return 0;
}
