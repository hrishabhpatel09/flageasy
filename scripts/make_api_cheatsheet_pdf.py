#!/usr/bin/env python3
"""Generate FlagEasy_API_Helpline.pdf — a two-line-per-function cheat sheet."""

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib.colors import HexColor
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    KeepTogether, HRFlowable,
)

ACCENT = HexColor("#1a4d8f")      # deep blue
ACCENT_LIGHT = HexColor("#eaf2fb")
CODE_BG = HexColor("#f4f4f2")
TEXT = HexColor("#222222")
MUTED = HexColor("#666666")

styles = {
    "title": ParagraphStyle("title", fontName="Helvetica-Bold", fontSize=26,
                            leading=30, textColor=ACCENT, spaceAfter=2),
    "subtitle": ParagraphStyle("subtitle", fontName="Helvetica", fontSize=11,
                               leading=15, textColor=MUTED),
    "h1": ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=15,
                         leading=19, textColor=HexColor("#ffffff"),
                         backColor=ACCENT, borderPadding=(4, 6, 4, 6),
                         spaceBefore=14, spaceAfter=8),
    "h2": ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=11.5,
                         leading=15, textColor=ACCENT, spaceBefore=10,
                         spaceAfter=3),
    "body": ParagraphStyle("body", fontName="Helvetica", fontSize=9.5,
                           leading=13, textColor=TEXT),
    "desc": ParagraphStyle("desc", fontName="Helvetica", fontSize=9.5,
                           leading=13, textColor=TEXT, spaceAfter=2),
    "code": ParagraphStyle("code", fontName="Courier", fontSize=8.5,
                           leading=11.5, textColor=HexColor("#0b3d2e"),
                           backColor=CODE_BG, borderPadding=(4, 5, 4, 5),
                           spaceBefore=1, spaceAfter=6),
    "fn": ParagraphStyle("fn", fontName="Courier-Bold", fontSize=9.5,
                         leading=13, textColor=ACCENT),
    "note": ParagraphStyle("note", fontName="Helvetica-Oblique", fontSize=9,
                           leading=12, textColor=MUTED, spaceAfter=4),
}


def fn_entry(signature, line1, line2, example=None):
    """One documented member function: signature, 2-line explanation, example."""
    flow = [
        Paragraph(signature.replace("&", "&amp;").replace("<", "&lt;")
                            .replace(">", "&gt;"), styles["fn"]),
        Paragraph(f"&bull; {line1}<br/>&bull; {line2}", styles["desc"]),
    ]
    if example:
        flow.append(Paragraph(example.replace("&", "&amp;")
                                      .replace("<", "&lt;")
                                      .replace(">", "&gt;"), styles["code"]))
    return KeepTogether(flow)


def section(title):
    return [
        Spacer(1, 4),
        Paragraph(title, styles["h1"]),
    ]


def hrule():
    return HRFlowable(width="100%", thickness=0.6, color=ACCENT_LIGHT,
                      spaceBefore=2, spaceAfter=2)


def header_footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(MUTED)
    canvas.drawString(18 * mm, 285 * mm, "FlagEasy v1.0 — API Helpline")
    canvas.drawRightString(A4[0] - 18 * mm, 285 * mm, "C++17 · flageasy")
    canvas.drawCentredString(A4[0] / 2, 12 * mm, f"page {doc.page}")
    canvas.setStrokeColor(ACCENT_LIGHT)
    canvas.line(18 * mm, 283 * mm, A4[0] - 18 * mm, 283 * mm)
    canvas.restoreState()


story = []

# ---------------------------------------------------------------- title page
story.append(Paragraph("FlagEasy", styles["title"]))
story.append(Paragraph("API Helpline — every member function, explained in two lines.",
                       styles["subtitle"]))
story.append(Spacer(1, 6))
story.append(Paragraph(
    "FlagEasy (<font face='Courier'>flageasy::</font>) is a C++17 command-line "
    "flag parsing library. Workflow: <b>define flags</b> on a "
    "<font face='Courier'>Parser</font> &rarr; <b>call</b> "
    "<font face='Courier'>parse(argc, argv)</font> &rarr; <b>read values</b> from "
    "the returned <font face='Courier'>ParseResult</font>. "
    "Header: <font face='Courier'>flageasy/flageasy.hpp</font>; link against "
    "<font face='Courier'>-lflageasy</font>.", styles["body"]))
story.append(Spacer(1, 4))

# The 4-line golden path
golden = (
    "flageasy::Parser cli(\"myapp\", \"Does useful things.\");\n"
    "cli.add_int(\"count\", 'c', \"Repeat count\").default_value(1).range(1, 100);\n"
    "auto r = cli.parse(argc, argv);          // never throws on bad user input\n"
    "if (r) { int n = r.as&lt;int&gt;(\"count\"); }"
)
story.append(Paragraph(golden, styles["code"]))

story.append(Paragraph(
    "Three public classes work together: <b>Parser</b> defines and runs the parse, "
    "<b>Flag</b> configures one flag fluently, <b>ParseResult</b> carries the outcome. "
    "Only definition-time programming errors (duplicate names, invalid identifiers) "
    "throw <font face='Courier'>std::invalid_argument</font> — everything else comes "
    "back inside the result.", styles["body"]))

# ---------------------------------------------------------------- Parser
story += section("1 · Parser — define flags, configure, parse")

story.append(Paragraph("Construction", styles["h2"]))
story.append(fn_entry(
    "Parser(std::string program_name, std::string description = {})",
    "Creates the parser; name appears in help/usage output (paths are reduced to a basename).",
    "The description is printed at the top of help().",
    "flageasy::Parser cli(\"myapp\", \"Does useful things.\");"))
story.append(Paragraph(
    "Copy is disabled; moving is allowed. Call <font face='Courier'>add_*</font> "
    "before parsing — flags added after parse() calls are not seen by earlier results.",
    styles["note"]))

story.append(Paragraph("Defining flags (each returns a Flag&amp; to configure fluently)", styles["h2"]))
story.append(fn_entry(
    "Flag& Parser::add_switch(std::string name, char short_name = '\\0', std::string help = {})",
    "Defines a boolean switch whose presence means true (--verbose).",
    "Supports negation (--verbose=false) and clustering (-vq); '\\0' means no short form.",
    "cli.add_switch(\"verbose\", 'v', \"Enable verbose output\");"))
story.append(fn_entry(
    "Flag& Parser::add_string(std::string name, char short_name = '\\0', std::string help = {})",
    "Defines an option that takes one text value.",
    "All conventional syntaxes work: --name v, --name=v, -n v, -nvalue, -n=v.",
    "cli.add_string(\"name\", 'n', \"Your name\").required();"))
story.append(fn_entry(
    "Flag& Parser::add_int(std::string name, char short_name = '\\0', std::string help = {})",
    "Defines an option parsed as a signed 64-bit integer.",
    "Pair with .range(low, high) and .default_value(n) for validation.",
    "cli.add_int(\"count\", 'c', \"Repeat count\").range(1, 100);"))
story.append(fn_entry(
    "Flag& Parser::add_double(std::string name, char short_name = '\\0', std::string help = {})",
    "Defines an option parsed as a double-precision float.",
    "Accepts 2, 2.5, -0.5, 1e3; validate with .range().",
    "cli.add_double(\"scale\", 's', \"Scale factor\").range(0.0, 10.0);"))
story.append(fn_entry(
    "Flag& Parser::add_string_list(std::string name, char short_name = '\\0', std::string help = {})",
    "Defines a repeatable string option; every occurrence appends one value.",
    "Retrieve all values with r.as&lt;std::vector&lt;std::string&gt;&gt;(name).",
    "cli.add_string_list(\"tag\", 't', \"Tags\");  // -t a -t b"))
story.append(fn_entry(
    "Flag& Parser::add_int_list(std::string name, char short_name = '\\0', std::string help = {})",
    "Repeatable int option — each occurrence contributes one integer.",
    "Retrieve with r.as&lt;std::vector&lt;int&gt;&gt;(name).",
    "cli.add_int_list(\"id\");"))
story.append(fn_entry(
    "Flag& Parser::add_double_list(std::string name, char short_name = '\\0', std::string help = {})",
    "Repeatable double option — each occurrence contributes one double.",
    "Retrieve with r.as&lt;std::vector&lt;double&gt;&gt;(name).",
    "cli.add_double_list(\"weight\");"))
story.append(fn_entry(
    "Flag& Parser::add_positional(std::string name, std::string help = {})",
    "Defines a positional argument filled in CLI order; required by default.",
    "Values are stored as strings; as&lt;int&gt;() et al. convert on read.",
    "cli.add_positional(\"input\", \"Input file\");"))
story.append(fn_entry(
    "Flag& Parser::add_positional_rest(std::string name, std::string help = {})",
    "Defines a positional that collects every remaining argument (add at most one, last).",
    "Never required; an empty remainder reads as an empty vector.",
    "cli.add_positional_rest(\"files\", \"Everything else\");"))

story.append(Paragraph("Parser configuration (each returns Parser&amp;)", styles["h2"]))
story.append(fn_entry(
    "Parser& Parser::version(std::string text)",
    "Sets a version string and registers the built-in --version/-V switch.",
    "After a match, r.version_requested() is true.",
    "cli.version(\"myapp 1.0.0\");"))
story.append(fn_entry(
    "Parser& Parser::description(std::string text)",
    "Overrides/sets the text shown at the top of help().",
    "Also settable via the constructor.",
    "cli.description(\"Converts images.\");"))
story.append(fn_entry(
    "Parser& Parser::epilog(std::string text)",
    "Sets text appended verbatim at the bottom of help().",
    "Use for examples, notes, or exit-code documentation.",
    "cli.epilog(\"Exit codes: 0 ok, 2 bad usage.\");"))
story.append(fn_entry(
    "Parser& Parser::allow_unknown(bool on = true)",
    "Capture unrecognized flags and surplus positionals instead of failing.",
    "Retrieve them afterwards via r.unknown().",
    "cli.allow_unknown(true);"))

story.append(Paragraph("Introspection & output", styles["h2"]))
story.append(fn_entry(
    "const std::string& Parser::program_name() const noexcept",
    "Returns the program name used in usage/help output (basename form).",
    "Handy when building custom error messages.",
    "std::cout << cli.program_name();"))
story.append(fn_entry(
    "const std::string& Parser::version_text() const noexcept",
    "Returns the version string previously set with version().",
    "Empty string when no version was registered.",
    "std::cout << cli.version_text();"))
story.append(fn_entry(
    "std::vector<const Flag*> Parser::flags() const",
    "Returns every defined flag (including hidden ones and built-ins) in definition order.",
    "Use to dump or audit the current flag set.",
    "for (const flageasy::Flag* f : cli.flags()) { /* f->name() */ }"))
story.append(fn_entry(
    "std::string Parser::usage() const",
    "Builds the one-line usage string, e.g. 'Usage: myapp -o FILE [options] &lt;input&gt;'.",
    "Worth printing after an error.",
    "std::cerr << cli.usage() << \"\\n\";"))
story.append(fn_entry(
    "std::string Parser::help() const",
    "Builds the full help text: description, usage, positionals, options, epilog.",
    "Honours $COLUMNS for word wrapping; aligned columns.",
    "if (r.help_requested()) { std::cout << cli.help(); return 0; }"))

story.append(Paragraph("Parsing", styles["h2"]))
story.append(fn_entry(
    "ParseResult Parser::parse(int argc, const char* const argv[]) const",
    "Parses argv[1..] (argv[0] is treated as the program name and skipped).",
    "Never throws for bad user input — all problems are reported through the result.",
    "flageasy::ParseResult r = cli.parse(argc, argv);"))
story.append(fn_entry(
    "ParseResult Parser::parse(const std::vector<std::string>& args) const",
    "Same, but takes an argument list that does NOT include the program name.",
    "Ideal for tests and for parsing config/REPL input.",
    "auto r = cli.parse(std::vector<std::string>{\"-c\", \"3\"});"))
story.append(Paragraph(
    "parse() is const and thread-safe on a shared Parser; each call returns an "
    "independent, copyable result.", styles["note"]))

# ---------------------------------------------------------------- Flag
story += section("2 · Flag — configure one flag fluently")

story.append(Paragraph(
    "Obtained from every <font face='Courier'>add_*</font> call; each configurator "
    "returns <font face='Courier'>Flag&amp;</font> so calls chain.", styles["body"]))

story.append(Paragraph("Introspection", styles["h2"]))
story.append(fn_entry(
    "const std::string& name() const noexcept  ·  char short_name() const noexcept",
    "name() returns the long name; short_name() returns ' by 0' when none was set.",
    "Useful when iterating cli.flags().",
    "if (f->short_name() == 'v') /* verbose */"))
story.append(fn_entry(
    "ValueType type() const noexcept",
    "Returns the value kind: Switch, Integer, Double, or String.",
    "Match on it when writing generic flag-handling code.",
    "if (f->type() == flageasy::ValueType::Integer) /* ... */"))
story.append(fn_entry(
    "bool is_list() / is_positional() / is_rest() / is_required() / is_hidden() / takes_value() const noexcept",
    "A family of quick checks: repeatable? positional? rest-collector? required? hidden from help?",
    "takes_value() is false only for switches.",
    "bool needs_value = f->takes_value();"))
story.append(fn_entry(
    "const std::string& help_text() const noexcept",
    "Returns the flag's help text shown in help output.",
    "Empty string if none was given.",
    "std::cout << f->help_text();"))

story.append(Paragraph("Fluent configurators", styles["h2"]))
story.append(fn_entry(
    "Flag& Flag::help(std::string text)",
    "Sets/overrides the help text shown in help output.",
    "Alternative to passing help to add_*.",
    ".help(\"Overrides the help text\")"))
story.append(fn_entry(
    "Flag& Flag::metavar(std::string text)",
    "Sets the value placeholder shown in help (default derives from the name).",
    "Example: 'FMT' renders as --format FMT.",
    ".metavar(\"FMT\")"))
story.append(fn_entry(
    "Flag& Flag::required(bool on = true)",
    "Marks the flag mandatory; parse fails with MissingRequired when absent.",
    "On positionals it is on by default — use .required(false) to make one optional.",
    ".required()"))
story.append(fn_entry(
    "Flag& Flag::hidden(bool on = true)",
    "Omits the flag from generated usage/help output.",
    "The flag still parses normally.",
    ".hidden()"))
story.append(fn_entry(
    "Flag& Flag::default_value(bool|const char*|std::string|T value)",
    "Sets the value used when the flag is not given; still validated at parse time.",
    "Integral/floating overloads accept any arithmetic type. A bad default is caught immediately.",
    ".default_value(1)  .default_value(\"png\")"))
story.append(fn_entry(
    "Flag& Flag::default_values(std::vector<std::string> values)",
    "Sets defaults for a list flag (several values at once).",
    "Also validated.",
    ".default_values({\"a\", \"b\"})"))
story.append(fn_entry(
    "Flag& Flag::range(double low, double high)",
    "Restricts numeric values to [low, high] inclusive; outside values fail with InvalidValue.",
    "Applies to int/double flags and their defaults.",
    ".range(1, 100)"))
story.append(fn_entry(
    "Flag& Flag::choices(std::vector<std::string> allowed)",
    "Whitelists the raw token; anything else fails with InvalidValue.",
    "Great for enum-like options (and typo suggestions come free).",
    ".choices({\"png\", \"jpeg\", \"webp\"})"))
story.append(fn_entry(
    "Flag& Flag::validate(std::function<bool(const std::string&, std::string&)> fn)",
    "Installs a custom check over the raw token: return false and fill err to reject.",
    "Runs after type/range/choices checks.",
    ".validate([](const std::string& v, std::string& err) {\n"
    "    if (v == \"raw\") { err = \"raw is not supported\"; return false; }\n"
    "    return true;\n"
    "})"))

# ---------------------------------------------------------------- ParseResult
story += section("3 · ParseResult — read what the user gave you")

story.append(Paragraph("Status & errors", styles["h2"]))
story.append(fn_entry(
    "bool ok() const noexcept  ·  explicit operator bool() const noexcept",
    "True when the parse succeeded; lets you write if (r) { ... }.",
    "First thing to check after parse().",
    "if (r) { /* use values */ } else { /* report */ }"))
story.append(fn_entry(
    "const ParseError& error() const noexcept",
    "Returns the structured failure: code (ErrorCode), message, flag_name, token.",
    "Empty (ErrorCode::None) after a successful parse.",
    "flageasy::ErrorCode c = r.error().code;"))
story.append(fn_entry(
    "const std::string& error_message() const noexcept",
    "One-line human-readable error, including typo suggestions.",
    "Empty after a successful parse.",
    "std::cerr << r.error_message() << \"\\n\";"))
story.append(fn_entry(
    "bool help_requested() const noexcept",
    "True when the user passed the built-in --help/-h.",
    "Print cli.help() and exit cleanly.",
    "if (r.help_requested()) { std::cout << cli.help(); }"))
story.append(fn_entry(
    "bool version_requested() const noexcept",
    "True when the user passed --version/-V (needs cli.version(...) set).",
    "Print your version string and exit.",
    "if (r.version_requested()) { std::cout << cli.version_text(); }"))

story.append(Paragraph("Raw access", styles["h2"]))
story.append(fn_entry(
    "bool has(const std::string& name) const noexcept",
    "Is a flag (or positional) with this long name defined?",
    "Guards against typos before deeper queries.",
    "if (r.has(\"count\")) /* ... */"))
story.append(fn_entry(
    "bool is_set(const std::string& name) const noexcept",
    "Was the flag explicitly provided on the command line?",
    "Lets you distinguish user-supplied values from defaults.",
    "if (r.is_set(\"verbose\")) /* user passed -v */"))
story.append(fn_entry(
    "const std::vector<std::string>& raw(const std::string& name) const",
    "Returns the effective raw values with defaults applied.",
    "Throws std::out_of_range for an unknown name.",
    "auto vals = r.raw(\"tag\");"))
story.append(fn_entry(
    "std::optional<std::string> value(const std::string& name) const",
    "Returns the first effective raw value, or nullopt when there is none.",
    "Convenience over raw()[0].",
    "auto v = r.value(\"name\");"))

story.append(Paragraph("Typed access", styles["h2"]))
story.append(fn_entry(
    "template<class T> T as(const std::string& name) const",
    "Converts to T: bool, std::string, arithmetic types, or std::vector of those.",
    "Throws std::runtime_error when unknown/missing/multi-value/unconvertible.",
    "int c = r.as<int>(\"count\");"))
story.append(fn_entry(
    "template<class T> std::optional<T> try_as(const std::string& name) const",
    "Like as&lt;T&gt; but returns nullopt instead of throwing.",
    "Best for optional or possibly-invalid input.",
    "auto d = r.try_as<double>(\"scale\");"))
story.append(fn_entry(
    "template<class T> T as_or(const std::string& name, T fallback) const",
    "Like as&lt;T&gt; but silently falls back to the given value on any failure.",
    "One-liner for 'value or my default'.",
    "int c = r.as_or<int>(\"count\", 5);"))
story.append(fn_entry(
    "auto v = r.as<std::vector<std::string>>(\"tag\");  // list flags",
    "Vector retrieval gathers every occurrence of a list flag in CLI order.",
    "Scalar as&lt;T&gt; on a multi-value flag throws — use the vector form.",
    "auto tags = r.as<std::vector<std::string>>(\"tag\");"))

story.append(Paragraph("Enumeration & reporting", styles["h2"]))
story.append(fn_entry(
    "const std::vector<ParsedFlag>& flags() const noexcept",
    "Every defined flag/positional in definition order with effective values and is_set markers.",
    "Includes the built-in help/version flags — great for dumping/debugging.",
    "for (const auto& pf : r.flags()) std::cout << pf.name << \"=\" << pf.value();"))
story.append(fn_entry(
    "const std::vector<std::string>& positionals() const noexcept",
    "Positional values in the order they appeared on the command line.",
    "Doubles as a quick lookup for rest-collectors.",
    "for (const auto& p : r.positionals()) /* ... */"))
story.append(fn_entry(
    "const std::vector<std::string>& unknown() const noexcept",
    "Arguments captured because allow_unknown(true) was set.",
    "Empty unless unknown-capture is enabled.",
    "for (const auto& u : r.unknown()) /* ... */"))
story.append(fn_entry(
    "std::vector<std::string> names() const",
    "Returns the names of all defined flags/positionals.",
    "Handy for validation loops or diagnostics.",
    "for (const auto& n : r.names()) /* ... */"))
story.append(fn_entry(
    "std::string to_string() const",
    "One-line summary like 'count=3, verbose=true, name=&lt;unset&gt;'.",
    "Perfect for logs and --dry-run style debugging.",
    "std::cout << r.to_string() << \"\\n\";"))

story.append(Paragraph("ParsedFlag fields", styles["h2"]))
story.append(Paragraph(
    "Each <font face='Courier'>ParsedFlag</font> exposes: <font face='Courier'>name</font> "
    "(long name), <font face='Courier'>short_name</font>, <font face='Courier'>type</font>, "
    "<font face='Courier'>is_positional</font>, <font face='Courier'>is_set</font>, "
    "<font face='Courier'>values</font> (effective), and "
    "<font face='Courier'>value()</font> — the first value or \"\" when there is none.",
    styles["body"]))

# ---------------------------------------------------------------- misc
story += section("4 · Free functions & error codes")

story.append(fn_entry(
    "namespace flageasy { const char* library_version() noexcept; / _major / _minor / _patch }",
    "Reports the library version as a string and as int components.",
    "Use for runtime capability checks or logs.",
    "std::cout << flageasy::library_version();  // \"1.0.0\""))
story.append(fn_entry(
    "std::string_view to_string(ValueType)  ·  std::string_view to_string(ErrorCode)",
    "Converts the enums to stable textual names ('Switch', 'UnknownFlag', ...).",
    "Use when printing diagnostics.",
    "std::cout << flageasy::to_string(r.error().code);"))

# error code table
err_rows = [
    ["ErrorCode", "Meaning"],
    ["None", "no error — parse succeeded"],
    ["UnknownFlag", "unrecognized --flag or -f"],
    ["MissingValue", "an option required a value that was not given"],
    ["InvalidValue", "value failed type / range / choices / validator"],
    ["MissingRequired", "a required flag or positional was not provided"],
    ["UnexpectedArgument", "extra positional with no slot for it"],
]
t = Table(err_rows, colWidths=[45 * mm, 120 * mm])
t.setStyle(TableStyle([
    ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
    ("FONTNAME", (0, 1), (0, -1), "Courier"),
    ("FONTNAME", (1, 1), (-1, -1), "Helvetica"),
    ("FONTSIZE", (0, 0), (-1, -1), 9),
    ("BACKGROUND", (0, 0), (-1, 0), ACCENT),
    ("TEXTCOLOR", (0, 0), (-1, 0), HexColor("#ffffff")),
    ("ROWBACKGROUNDS", (0, 1), (-1, -1), [HexColor("#ffffff"), ACCENT_LIGHT]),
    ("GRID", (0, 0), (-1, -1), 0.4, HexColor("#c8d6e5")),
    ("TOPPADDING", (0, 0), (-1, -1), 3),
    ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ("LEFTPADDING", (0, 0), (-1, -1), 6),
]))
story.append(Spacer(1, 6))
story.append(t)
story.append(Spacer(1, 6))

story.append(Paragraph("Standard failure-handling recipe", styles["h2"]))
story.append(Paragraph(
    "if (r.help_requested()) { std::cout &lt;&lt; cli.help(); return 0; }\n"
    "if (r.version_requested()) { std::cout &lt;&lt; cli.version_text(); return 0; }\n"
    "if (!r) { std::cerr &lt;&lt; \"error: \" &lt;&lt; r.error_message() &lt;&lt; \"\\n\\n\"\n"
    "          &lt;&lt; cli.usage() &lt;&lt; \"\\n\"; return 2; }",
    styles["code"]))

story.append(hrule())
story.append(Paragraph(
    "FlagEasy v1.0.0 · header: flageasy/flageasy.hpp · build: "
    "cmake --install build --prefix &lt;prefix&gt;, then find_package(flageasy 1.0 REQUIRED) "
    "and link flageasy::flageasy. Only definition-time misuse throws "
    "(std::invalid_argument); bad user input never throws.",
    styles["note"]))


def main():
    doc = SimpleDocTemplate(
        "FlagEasy_API_Helpline.pdf", pagesize=A4,
        leftMargin=18 * mm, rightMargin=18 * mm,
        topMargin=22 * mm, bottomMargin=18 * mm,
        title="FlagEasy API Helpline",
        author="FlagEasy",
        subject="Member-function cheat sheet for the flageasy C++17 library",
    )
    doc.build(story, onFirstPage=header_footer, onLaterPages=header_footer)
    print("wrote FlagEasy_API_Helpline.pdf")


if __name__ == "__main__":
    main()
