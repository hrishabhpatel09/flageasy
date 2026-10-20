# FlagEasy

A small, robust command-line flag parsing library for C++17, shipped as a
macOS dynamic library (`libflageasy.dylib`).

Define your flags with a fluent API, hand it `argc/argv`, and get back a
result that tells you exactly which flags were set and what values they
carry — as raw strings or converted to any type you ask for.

```cpp
#include <flageasy/flageasy.hpp>

int main(int argc, char** argv) {
    flageasy::Parser cli("myapp", "Does useful things.");
    cli.version("myapp 1.0.0");

    cli.add_switch("verbose", 'v', "Enable verbose output");
    cli.add_int("count", 'c', "Repeat count").default_value(1).range(1, 100);
    cli.add_string("output", 'o', "Output file").required();
    cli.add_positional("input", "Input file");

    flageasy::ParseResult r = cli.parse(argc, argv);
    if (r.help_requested()) { std::cout << cli.help(); return 0; }
    if (!r.ok()) {
        std::cerr << "error: " << r.error_message() << "\n\n" << cli.usage() << "\n";
        return 2;
    }

    int count  = r.as<int>("count");
    bool quiet = !r.as<bool>("verbose");
    std::string input = r.as<std::string>("input");
}
```

## Features

- **Switches, options, lists, positionals** — `add_switch`, `add_string`,
  `add_int`, `add_double`, `add_string_list`, `add_int_list`,
  `add_double_list`, `add_positional`, `add_positional_rest`.
- **All conventional syntaxes** — `--flag value`, `--flag=value`, `-f value`,
  `-fvalue`, `-f=value`, clustered switches (`-vq`), negated switches
  (`--verbose=false`), `--` terminator, negative numbers as values.
- **Validation built in** — required flags, defaults (validated at parse
  time), ranges, choices, and custom validator callbacks. Default values are
  checked too, so a bad default is caught immediately.
- **Typed retrieval** — `r.as<int>("count")`, `r.as<std::vector<std::string>>("tag")`,
  `r.try_as<double>(...)`, `r.as_or<int>(..., fallback)`.
- **Full enumeration** — `r.flags()` lists every flag (including built-ins and
  positionals) with `is_set` and effective values; `r.to_string()` gives a
  one-line summary.
- **Generated help** — aligned, word-wrapped usage/help text that honours
  `$COLUMNS`, built-in `--help/-h` and `--version/-V`.
- **Friendly errors** — machine-readable `ErrorCode` plus human-readable
  messages with typo suggestions (`did you mean '--quality'?`).
- **Never throws for user input** — bad user input is reported through
  `ParseResult`. Only definition-time programming errors throw
  (`std::invalid_argument`).
- **Dylib hygiene** — `-fvisibility=hidden` with explicit exports, `@rpath`
  install name, `SOVERSION`, pimpl'd public classes, CMake package export.

## Building

Requires CMake ≥ 3.16 and any C++17 compiler (tested with Apple clang 21 on
arm64 macOS).

```sh
./scripts/build.sh          # configure + build + test into ./build
# or manually:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Products:

```
build/libflageasy.1.0.0.dylib   # the library
build/libflageasy.1.dylib       # SOVERSION symlink
build/libflageasy.dylib         # dev symlink
build/examples/flageasy_demo    # usage demo
build/tests/flageasy_tests      # test suite
```

Try the demo:

```sh
./build/examples/flageasy_demo --help
./build/examples/flageasy_demo -q 42 -f jpeg -o out.jpg --scale 2.5 -t beta in.png
```

## Integrating into another project

### With CMake (recommended)

Install once:

```sh
cmake --install build --prefix /path/to/prefix
```

Then in the consuming project:

```cmake
find_package(flageasy 1.0 REQUIRED)
target_link_libraries(myapp PRIVATE flageasy::flageasy)
```

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

A complete standalone consumer lives in [`examples/consumer/`](examples/consumer/);
`./scripts/test_install.sh` installs into a throwaway prefix and builds it
end-to-end.

### Without CMake

```sh
clang++ -std=c++17 myapp.cpp \
    -I/path/to/FlagEasy/include \
    -L/path/to/FlagEasy/build -lflageasy \
    -Wl,-rpath,/path/to/FlagEasy/build \
    -o myapp
```

The rpath is required because the dylib's install name is
`@rpath/libflageasy.1.dylib`. For apps you ship, point the rpath at wherever
the dylib will live (commonly `@loader_path/../lib`, and `/usr/local/lib` is
searched by dyld by default).

## API overview

### Defining flags

Every `add_*` returns a `Flag&` you can configure fluently:

```cpp
cli.add_string("format", 'f', "Output format")
    .help("Overrides the help text")       // or pass it to add_*
    .metavar("FMT")                        // placeholder in help output
    .choices({"png", "jpeg", "webp"})      // raw-token whitelist
    .default_value("png")
    .required()                            // or .required(false)
    .hidden()                              // omit from help/usage
    .range(0.0, 1.0)                       // numeric flags only
    .validate([](const std::string& v, std::string& err) {
        if (v == "raw") { err = "raw is not supported"; return false; }
        return true;
    });
```

Positionals are required by default; call `.required(false)` for an optional
one. A rest positional (`add_positional_rest`) collects everything left and
is never required. The names `help`/`h` are reserved for the built-in help
flag, and `version`/`V` for the built-in version flag once `cli.version(...)`
is called.

### Reading the result

```cpp
flageasy::ParseResult r = cli.parse(argc, argv);

r.ok();                         // bool (or `if (r)`)
r.error().code;                 // flageasy::ErrorCode
r.error_message();              // "unknown flag '--qualit' (did you mean ...?)"
r.help_requested();             // -h/--help seen
r.version_requested();          // -V/--version seen

r.is_set("count");              // provided on the command line?
r.raw("count");                 // std::vector<std::string> of effective values
r.value("count");               // std::optional<std::string>, first value

r.as<int>("count");             // throws std::runtime_error on failure
r.try_as<int>("count");         // std::optional<int>, never throws
r.as_or<int>("count", 5);       // fallback value

r.as<std::vector<std::string>>("tag");
r.flags();                      // every ParsedFlag: name, values, is_set, ...
r.positionals();                // positional values in CLI order
r.unknown();                    // captured with allow_unknown(true)
r.to_string();                  // "count=3, verbose=true, name=<unset>"
```

An unset switch reads as `false`; unset flags with defaults read as the
default (with `is_set == false` so you can tell them apart).

### Parser configuration

```cpp
cli.version("myapp 1.2.3");     // adds --version/-V
cli.description("...");         // shown at the top of help()
cli.epilog("...");              // appended at the bottom of help()
cli.allow_unknown(true);        // capture unknown flags instead of failing
cli.usage();                    // one-line usage string
cli.help();                     // full help text
```

## Command-line rules in detail

| Input | Meaning |
| --- | --- |
| `--name value` | value follows |
| `--name=value` | inline value (`--name=` yields an empty string) |
| `-n value`, `-nvalue`, `-n=value` | short forms |
| `-vq` | clustered switches |
| `--verbose=false` / `-v=off` | negate a switch (`true/false, yes/no, on/off, 1/0`) |
| `-5`, `-.5` | treated as values, not flags |
| `-` | a value (stdin convention) |
| `--` | everything after it is positional |
| `--name -x` | error: use `--name=-x` for values that look like flags |
| extra occurrences | non-list flags keep the last value; list flags append |
| unknown flag | error, or captured with `allow_unknown(true)` |

## Design notes

- `parse()` is `const` and thread-safe on a shared `Parser`; each call returns
  an independent, copyable `ParseResult`.
- Public classes are pimpl'd and built with hidden visibility, so the dylib
  exports only `flageasy::*` API symbols (88 symbols in the current build)
  and internal changes can be made without breaking the ABI; `SOVERSION` is 1.
- Conversion uses `std::from_chars` when available with a `strtoll`/`strtod`
  fallback; doubles are printed shortest-round-trip via `std::to_chars`.

## Roadmap

Not in v1, kept deliberately out to stay small: subcommands, environment
variable fallbacks, config-file support, and a C ABI wrapper. The internals
(pimpl + `internal.hpp`) are structured so these can be added without
breaking the public API.

## Project layout

```
include/flageasy/flageasy.hpp   public header (installed)
src/                            implementation (hidden visibility)
tests/                          self-contained test runner (391 checks)
examples/demo.cpp               annotated usage demo
examples/consumer/              standalone find_package consumer
scripts/build.sh                build + test helper
scripts/test_install.sh         install + consumer end-to-end check
```
