// flageasy — Parser implementation: flag definitions and the parse loop.

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "internal.hpp"

namespace flageasy {
namespace {

// --- typo suggestions -------------------------------------------------------

std::size_t edit_distance(const std::string& a, const std::string& b) {
    const std::size_t n = b.size();
    std::vector<std::size_t> previous(n + 1);
    std::vector<std::size_t> current(n + 1);
    for (std::size_t j = 0; j <= n; ++j) previous[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        current[0] = i;
        for (std::size_t j = 1; j <= n; ++j) {
            const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0u : 1u;
            current[j] = std::min({previous[j] + 1u, current[j - 1] + 1u,
                                   previous[j - 1] + cost});
        }
        previous.swap(current);
    }
    return previous[n];
}

std::string suggest_long(const std::vector<std::unique_ptr<Flag>>& flags,
                         const std::string& name) {
    std::string best;
    std::size_t best_distance = std::string::npos;
    for (const auto& flag : flags) {
        if (flag->is_positional()) continue;
        const std::size_t distance = edit_distance(name, flag->name());
        if (distance < best_distance) {
            best_distance = distance;
            best = flag->name();
        }
    }
    if (best.empty()) return {};
    const std::size_t limit =
        std::min<std::size_t>(3, name.size() / 3 + 1);
    if (best_distance > limit) return {};
    return " (did you mean '--" + best + "'?)";
}

std::string suggest_short(const std::vector<std::unique_ptr<Flag>>& flags,
                          char c) {
    const unsigned char lower =
        static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(c)));
    for (const auto& flag : flags) {
        const char s = flag->short_name();
        if (s == '\0') continue;
        if (static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(s))) == lower) {
            return std::string(" (did you mean '-") + s + "'?)";
        }
    }
    return {};
}

/// A token that looks like an option rather than a value: starts with '-',
/// has more than one character, and is not a negative number ("-5", "-.5").
/// A lone "-" is treated as a value (stdin convention).
bool is_option_like(const std::string& token) {
    if (token.size() < 2 || token[0] != '-') return false;
    const char second = token[1];
    if (std::isdigit(static_cast<unsigned char>(second)) || second == '.')
        return false;
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction / movement
// ---------------------------------------------------------------------------

Parser::Parser(std::string program_name, std::string description)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->program =
        program_name.empty() ? "program" : internal::basename_of(program_name);
    m_impl->description = std::move(description);

    // Built-in help flag, always available.
    Flag& help_flag = add_flag("help", 'h', "Show this help message and exit",
                               ValueType::Switch, false, false, false);
    m_impl->help_flag = &help_flag;
}

Parser::~Parser() = default;
Parser::Parser(Parser&& other) noexcept = default;
Parser& Parser::operator=(Parser&& other) noexcept = default;

// ---------------------------------------------------------------------------
// Flag definition
// ---------------------------------------------------------------------------

Flag& Parser::add_flag(std::string name, char short_name, std::string help,
                       ValueType type, bool list, bool positional, bool rest) {
    Impl& defs = *m_impl;

    if (!internal::is_valid_flag_name(name))
        throw std::invalid_argument(
            "flageasy: invalid flag name '" + name +
            "' (use letters, digits, '-', '_', '.'; start alphanumeric)");
    if (defs.by_name.count(name) != 0u)
        throw std::invalid_argument("flageasy: duplicate flag name '--" + name + "'");
    if (positional && short_name != '\0')
        throw std::invalid_argument(
            "flageasy: positional arguments cannot have a short name");
    if (short_name != '\0') {
        if (short_name == '-' || short_name == '=')
            throw std::invalid_argument(std::string("flageasy: reserved short flag '-") +
                                        short_name + "'");
        if (defs.by_short.count(short_name) != 0u)
            throw std::invalid_argument(std::string("flageasy: duplicate short flag '-") +
                                        short_name + "'");
    }
    if (rest && defs.rest_flag != nullptr)
        throw std::invalid_argument(
            "flageasy: only one rest positional is allowed");
    if (positional && defs.rest_flag != nullptr)
        throw std::invalid_argument("flageasy: positional '" + name +
                                    "' cannot be added after a rest positional");

    auto impl = std::make_unique<Flag::Impl>();
    impl->name = std::move(name);
    impl->short_name = short_name;
    impl->type = type;
    impl->list = list || rest;
    impl->positional = positional;
    impl->rest = rest;
    impl->required = positional && !rest;  // positionals required by default
    impl->help = std::move(help);

    auto holder = std::unique_ptr<Flag>(new Flag(std::move(impl)));
    Flag* flag = holder.get();

    defs.by_name.emplace(flag->m_impl->name, flag);
    if (short_name != '\0') defs.by_short.emplace(short_name, flag);
    if (positional) {
        defs.positionals.push_back(flag);
        if (rest) defs.rest_flag = flag;
    }
    defs.flags.push_back(std::move(holder));
    return *flag;
}

Flag& Parser::add_switch(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::Switch, false, false, false);
}

Flag& Parser::add_string(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::String, false, false, false);
}

Flag& Parser::add_int(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::Integer, false, false, false);
}

Flag& Parser::add_double(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::Double, false, false, false);
}

Flag& Parser::add_string_list(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::String, true, false, false);
}

Flag& Parser::add_int_list(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::Integer, true, false, false);
}

Flag& Parser::add_double_list(std::string name, char short_name, std::string help) {
    return add_flag(std::move(name), short_name, std::move(help),
                    ValueType::Double, true, false, false);
}

Flag& Parser::add_positional(std::string name, std::string help) {
    return add_flag(std::move(name), '\0', std::move(help), ValueType::String,
                    false, true, false);
}

Flag& Parser::add_positional_rest(std::string name, std::string help) {
    return add_flag(std::move(name), '\0', std::move(help), ValueType::String,
                    true, true, true);
}

// ---------------------------------------------------------------------------
// Configuration / introspection
// ---------------------------------------------------------------------------

Parser& Parser::version(std::string text) {
    m_impl->version_text = std::move(text);
    if (m_impl->version_flag == nullptr) {
        Flag* existing = m_impl->find_long("version");
        if (existing != nullptr) {
            m_impl->version_flag = existing;  // user defined their own
        } else {
            const char short_name =
                m_impl->find_short('V') == nullptr ? 'V' : '\0';
            Flag& flag = add_switch("version", short_name,
                                    "Show version information and exit");
            m_impl->version_flag = &flag;
        }
    }
    return *this;
}

Parser& Parser::description(std::string text) {
    m_impl->description = std::move(text);
    return *this;
}

Parser& Parser::epilog(std::string text) {
    m_impl->epilog = std::move(text);
    return *this;
}

Parser& Parser::allow_unknown(bool on) {
    m_impl->allow_unknown = on;
    return *this;
}

const std::string& Parser::program_name() const noexcept {
    return m_impl->program;
}

const std::string& Parser::version_text() const noexcept {
    return m_impl->version_text;
}

std::vector<const Flag*> Parser::flags() const {
    std::vector<const Flag*> out;
    out.reserve(m_impl->flags.size());
    for (const auto& flag : m_impl->flags) out.push_back(flag.get());
    return out;
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

ParseResult Parser::parse(int argc, const char* const argv[]) const {
    std::vector<std::string> args;
    if (argc > 1) args.reserve(static_cast<std::size_t>(argc - 1));
    for (int i = 1; i < argc; ++i)
        if (argv[i] != nullptr) args.emplace_back(argv[i]);
    return parse(args);
}

ParseResult Parser::parse(const std::vector<std::string>& args) const {
    const Impl& defs = *m_impl;
    ParseResult result;
    ParseResult::Impl& out = *result.m_impl;

    // --- result initialization: one entry per defined flag --------------------
    std::unordered_map<const Flag*, std::size_t> def_index;
    out.flags.reserve(defs.flags.size());
    for (std::size_t k = 0; k < defs.flags.size(); ++k) {
        const Flag::Impl& f = *defs.flags[k]->m_impl;
        ParsedFlag entry;
        entry.name = f.name;
        entry.short_name = f.short_name;
        entry.type = f.type;
        entry.is_positional = f.positional;
        if (f.has_default)
            entry.values = f.defaults;
        else if (f.type == ValueType::Switch)
            entry.values.assign(1, "false");
        out.index.emplace(f.name, k);
        out.flags.push_back(std::move(entry));
        def_index.emplace(defs.flags[k].get(), k);
    }

    // --- helpers ----------------------------------------------------------------
    const auto fail = [&](ErrorCode code, std::string message,
                          std::string flag_name = {}, std::string token = {}) {
        out.ok = false;
        out.error.code = code;
        out.error.message = std::move(message);
        out.error.flag_name = std::move(flag_name);
        out.error.token = std::move(token);
    };

    const auto display_name = [](const Flag::Impl& f) -> std::string {
        return f.positional ? ("'" + f.name + "'") : ("--" + f.name);
    };

    const auto range_desc = [](const Flag::Impl& f) -> std::string {
        if (f.type == ValueType::Integer) {
            return "[" + std::to_string(static_cast<long long>(f.range_lo)) +
                   ", " + std::to_string(static_cast<long long>(f.range_hi)) +
                   "]";
        }
        return "[" + internal::format_number(f.range_lo) + ", " +
               internal::format_number(f.range_hi) + "]";
    };

    const auto check_value = [&](const Flag::Impl& f, const std::string& value,
                                 std::string& error) -> bool {
        if (f.type == ValueType::Integer) {
            long long parsed = 0;
            if (!detail::convert_int(value, parsed, error)) return false;
            if (f.has_range &&
                (static_cast<double>(parsed) < f.range_lo ||
                 static_cast<double>(parsed) > f.range_hi)) {
                error = "value '" + value + "' is outside the allowed range " +
                        range_desc(f);
                return false;
            }
        } else if (f.type == ValueType::Double) {
            double parsed = 0.0;
            if (!detail::convert_double(value, parsed, error)) return false;
            if (f.has_range && (parsed < f.range_lo || parsed > f.range_hi)) {
                error = "value '" + value + "' is outside the allowed range " +
                        range_desc(f);
                return false;
            }
        } else if (f.type == ValueType::Switch) {
            bool parsed = false;
            if (!detail::convert_bool(value, parsed, error)) return false;
        }
        if (!f.choices.empty()) {
            bool found = false;
            for (const std::string& choice : f.choices) {
                if (choice == value) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::string allowed;
                for (std::size_t k = 0; k < f.choices.size(); ++k) {
                    if (k != 0) allowed += ", ";
                    allowed += f.choices[k];
                }
                error = "value '" + value + "' is not one of: " + allowed;
                return false;
            }
        }
        if (f.validator) {
            std::string custom;
            if (!f.validator(value, custom)) {
                error = custom.empty() ? ("invalid value '" + value + "'")
                                       : custom;
                return false;
            }
        }
        return true;
    };

    const auto assign = [&](const Flag& flag, std::string value) {
        const Flag::Impl& f = *flag.m_impl;
        ParsedFlag& entry = out.flags[def_index.at(&flag)];
        if (f.list && entry.is_set)
            entry.values.push_back(std::move(value));
        else
            entry.values.assign(1, std::move(value));
        entry.is_set = true;
    };

    const auto validate_and_assign = [&](const Flag& flag,
                                         const std::string& value) -> bool {
        const Flag::Impl& f = *flag.m_impl;
        std::string error;
        if (!check_value(f, value, error)) {
            fail(ErrorCode::InvalidValue,
                 error + " for flag " + display_name(f), f.name, value);
            return false;
        }
        assign(flag, value);
        return true;
    };

    // --- developer feedback: defaults must pass validation ----------------------
    for (const auto& holder : defs.flags) {
        const Flag::Impl& f = *holder->m_impl;
        if (!f.has_default) continue;
        for (const std::string& candidate : f.defaults) {
            std::string error;
            if (!check_value(f, candidate, error)) {
                fail(ErrorCode::InvalidValue,
                     "invalid default value '" + candidate + "' for flag " +
                         display_name(f) + ": " + error,
                     f.name, candidate);
                return result;
            }
        }
    }

    // --- parse loop ---------------------------------------------------------------
    std::size_t i = 0;
    std::size_t next_positional = 0;
    bool positional_only = false;

    const auto take_next_value = [&](const Flag& flag, std::string& value,
                                     const std::string& token) -> bool {
        const std::string display = "--" + flag.m_impl->name;
        if (i + 1 >= args.size()) {
            fail(ErrorCode::MissingValue,
                 "flag '" + display + "' requires a value", flag.m_impl->name,
                 token);
            return false;
        }
        const std::string& next = args[i + 1];
        if (is_option_like(next)) {
            fail(ErrorCode::MissingValue,
                 "flag '" + display + "' requires a value (got '" + next +
                     "'); use '" + display +
                     "=VALUE' when the value starts with '-'",
                 flag.m_impl->name, token);
            return false;
        }
        value = next;
        ++i;
        return true;
    };

    const auto assign_positional = [&](const std::string& token) -> bool {
        if (next_positional < defs.positionals.size()) {
            const Flag& flag = *defs.positionals[next_positional];
            if (!validate_and_assign(flag, token)) return false;
            if (!flag.m_impl->rest) ++next_positional;
            out.positionals.push_back(token);
            return true;
        }
        if (defs.allow_unknown) {
            out.unknown.push_back(token);
            return true;
        }
        fail(ErrorCode::UnexpectedArgument, "unexpected argument '" + token + "'",
             {}, token);
        return false;
    };

    for (; i < args.size(); ++i) {
        const std::string& token = args[i];

        if (positional_only) {
            if (!assign_positional(token)) return result;
            continue;
        }
        if (token == "--") {
            positional_only = true;
            continue;
        }

        // --- long option: --name, --name=value ----------------------------------
        if (token.size() >= 2 && token[0] == '-' && token[1] == '-') {
            std::string_view body(token);
            body.remove_prefix(2);
            std::string_view inline_value;
            bool has_inline = false;
            const std::size_t eq = body.find('=');
            if (eq != std::string_view::npos) {
                inline_value = body.substr(eq + 1);
                body = body.substr(0, eq);
                has_inline = true;
            }
            const std::string name(body);
            Flag* flag = defs.find_long(name);
            if (flag == nullptr) {
                if (defs.allow_unknown) {
                    out.unknown.push_back(token);
                    continue;
                }
                fail(ErrorCode::UnknownFlag,
                     "unknown flag '--" + name + "'" +
                         suggest_long(defs.flags, name),
                     name, token);
                return result;
            }
            const Flag::Impl& f = *flag->m_impl;
            if (f.type == ValueType::Switch) {
                bool value = true;
                if (has_inline) {
                    std::string error;
                    if (!detail::convert_bool(std::string(inline_value), value,
                                              error)) {
                        fail(ErrorCode::InvalidValue,
                             error + " for flag --" + f.name, f.name, token);
                        return result;
                    }
                }
                if (!validate_and_assign(*flag, value ? "true" : "false"))
                    return result;
            } else if (has_inline) {
                if (!validate_and_assign(*flag, std::string(inline_value)))
                    return result;
            } else {
                std::string value;
                if (!take_next_value(*flag, value, token)) return result;
                if (!validate_and_assign(*flag, value)) return result;
            }
            continue;
        }

        // --- short option(s): -v, -abc, -nVALUE, -n=VALUE, -v=false ---------------
        if (token.size() >= 2 && token[0] == '-') {
            for (std::size_t j = 1; j < token.size(); ++j) {
                const char c = token[j];
                Flag* flag = defs.find_short(c);
                if (flag == nullptr) {
                    if (defs.allow_unknown) {
                        out.unknown.push_back("-" + token.substr(j));
                        break;  // remainder captured as unknown
                    }
                    const std::string short_text(1, c);
                    fail(ErrorCode::UnknownFlag,
                         "unknown flag '-" + short_text + "'" +
                             suggest_short(defs.flags, c),
                         short_text, token);
                    return result;
                }
                const Flag::Impl& f = *flag->m_impl;
                if (f.type == ValueType::Switch) {
                    bool value = true;
                    if (j + 1 < token.size() && token[j + 1] == '=') {
                        std::string error;
                        if (!detail::convert_bool(token.substr(j + 2), value,
                                                  error)) {
                            fail(ErrorCode::InvalidValue,
                                 error + " for flag -" + std::string(1, c),
                                 f.name, token);
                            return result;
                        }
                        j = token.size();  // consume the rest
                    }
                    if (!validate_and_assign(*flag, value ? "true" : "false"))
                        return result;
                } else {
                    std::string rest = token.substr(j + 1);
                    if (!rest.empty() && rest[0] == '=') rest.erase(0, 1);
                    if (!rest.empty()) {
                        if (!validate_and_assign(*flag, rest)) return result;
                    } else {
                        std::string value;
                        if (!take_next_value(*flag, value, token))
                            return result;
                        if (!validate_and_assign(*flag, value)) return result;
                    }
                    break;  // the value consumed the rest of the cluster
                }
            }
            continue;
        }

        // --- positional -------------------------------------------------------------
        if (!assign_positional(token)) return result;
    }

    // --- built-in help / version --------------------------------------------------
    const auto is_set_flag = [&](const Flag* flag) -> bool {
        if (flag == nullptr) return false;
        const auto it = def_index.find(flag);
        return it != def_index.end() && out.flags[it->second].is_set;
    };
    out.help = is_set_flag(defs.help_flag);
    out.version = is_set_flag(defs.version_flag);

    // --- required checks (skipped when help/version was requested) -----------------
    if (!out.help && !out.version) {
        for (std::size_t k = 0; k < defs.flags.size(); ++k) {
            const Flag::Impl& f = *defs.flags[k]->m_impl;
            if (!f.required || out.flags[k].is_set) continue;
            if (f.positional)
                fail(ErrorCode::MissingRequired,
                     "missing required argument '" + f.name + "'", f.name);
            else
                fail(ErrorCode::MissingRequired,
                     "missing required flag --" + f.name, f.name);
            return result;
        }
    }

    return result;
}

}  // namespace flageasy
