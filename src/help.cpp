// flageasy — usage and help text generation.

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "internal.hpp"

namespace flageasy {
namespace {

int terminal_width() {
    int width = 80;
    if (const char* env = std::getenv("COLUMNS")) {
        char* end = nullptr;
        const long parsed = std::strtol(env, &end, 10);
        if (end != env && end != nullptr && *end == '\0' && parsed >= 50)
            width = static_cast<int>(std::min<long>(parsed, 200));
    }
    return width;
}

/// Word-wrap one paragraph.
std::vector<std::string> wrap_paragraph(const std::string& text, std::size_t width) {
    std::vector<std::string> lines;
    if (width < 10) width = 10;
    std::size_t start = 0;
    std::string current;
    while (start < text.size()) {
        while (start < text.size() && text[start] == ' ') ++start;
        if (start >= text.size()) break;
        std::size_t end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        const std::string word = text.substr(start, end - start);
        start = end;
        if (current.empty()) {
            current = word;
        } else if (current.size() + 1 + word.size() <= width) {
            current += ' ';
            current += word;
        } else {
            lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

/// Word-wrap text, honouring explicit '\n' as hard breaks.
std::vector<std::string> wrap(const std::string& text, std::size_t width) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t nl = text.find('\n', start);
        if (nl == std::string::npos) nl = text.size();
        const std::string paragraph = text.substr(start, nl - start);
        const std::vector<std::string> wrapped = wrap_paragraph(paragraph, width);
        lines.insert(lines.end(), wrapped.begin(), wrapped.end());
        if (nl == text.size()) break;
        start = nl + 1;
    }
    return lines;
}

std::string uppercase(std::string text) {
    for (char& c : text)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

std::string metavar_for(const Flag& flag) {
    const auto& f = internal::FlagAccess::impl(flag);
    if (!f.metavar.empty()) return f.metavar;
    return uppercase(f.name);
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (std::size_t k = 0; k < parts.size(); ++k) {
        if (k != 0) out += sep;
        out += parts[k];
    }
    return out;
}

std::string range_text(const Flag& flag) {
    const auto& f = internal::FlagAccess::impl(flag);
    if (f.type == ValueType::Integer) {
        return std::to_string(static_cast<long long>(f.range_lo)) + ".." +
               std::to_string(static_cast<long long>(f.range_hi));
    }
    return internal::format_number(f.range_lo) + ".." +
           internal::format_number(f.range_hi);
}

/// "(default: 1; range: 1..100)" or "" when there is nothing to add.
std::string notes_for(const Flag& flag) {
    const auto& f = internal::FlagAccess::impl(flag);
    std::vector<std::string> notes;
    if (f.required && !f.positional) notes.push_back("required");
    if (f.has_default) notes.push_back("default: " + join(f.defaults, ", "));
    if (!f.choices.empty()) notes.push_back("choices: " + join(f.choices, ", "));
    if (f.has_range) notes.push_back("range: " + range_text(flag));
    if (notes.empty()) return {};
    return "(" + join(notes, "; ") + ")";
}

std::string option_left(const Flag& flag) {
    const auto& f = internal::FlagAccess::impl(flag);
    std::string left;
    if (f.short_name != '\0') {
        left += '-';
        left += f.short_name;
        left += ", ";
    }
    left += "--" + f.name;
    if (f.type != ValueType::Switch) {
        left += ' ';
        left += metavar_for(flag);
        if (f.list) left += " ...";
    }
    return left;
}

std::string option_usage(const Flag& flag) {
    const auto& f = internal::FlagAccess::impl(flag);
    std::string out;
    if (f.short_name != '\0') {
        out += '-';
        out += f.short_name;
    } else {
        out += "--" + f.name;
    }
    if (f.type != ValueType::Switch) {
        out += ' ';
        out += metavar_for(flag);
        if (f.list) out += " ...";
    }
    return out;
}

/// Two-column layout: "  left   right" with the right column wrapped.
std::string render_rows(const std::vector<std::pair<std::string, std::string>>& rows,
                        int width) {
    std::size_t column = 0;
    for (const auto& row : rows) column = std::max(column, row.first.size());
    const std::size_t cap = 28;
    const bool align = column <= cap;
    if (!align) column = cap;
    const std::size_t help_indent = 2 + column + 2;

    std::string out;
    for (const auto& row : rows) {
        const std::string left = "  " + row.first;
        const std::size_t right_width =
            width > static_cast<int>(help_indent + 10)
                ? static_cast<std::size_t>(width) - help_indent
                : 10;
        const std::vector<std::string> right = wrap(row.second, right_width);

        if (left.size() <= 2 + column) {
            std::string line = left;
            line.resize(2 + column, ' ');
            line += "  ";
            if (!right.empty()) line += right.front();
            while (!line.empty() && line.back() == ' ') line.pop_back();
            out += line;
            out += '\n';
            for (std::size_t k = 1; k < right.size(); ++k) {
                out += std::string(help_indent, ' ');
                out += right[k];
                out += '\n';
            }
        } else {
            out += left;
            out += '\n';
            for (const std::string& line : right) {
                out += std::string(help_indent, ' ');
                out += line;
                out += '\n';
            }
        }
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// Parser::usage
// ---------------------------------------------------------------------------

std::string Parser::usage() const {
    const Impl& defs = *m_impl;
    std::string out = "Usage: " + defs.program;

    bool has_optional = false;
    for (const auto& holder : defs.flags) {
        const Flag::Impl& f = *holder->m_impl;
        if (f.positional || f.hidden) continue;
        if (f.required)
            out += " " + option_usage(*holder);
        else
            has_optional = true;
    }
    if (has_optional) out += " [options]";

    for (const Flag* flag : defs.positionals) {
        const Flag::Impl& f = *flag->m_impl;
        if (f.hidden) continue;
        if (f.rest)
            out += " [<" + f.name + "> ...]";
        else if (f.required)
            out += " <" + f.name + ">";
        else
            out += " [<" + f.name + ">]";
    }
    return out;
}

// ---------------------------------------------------------------------------
// Parser::help
// ---------------------------------------------------------------------------

std::string Parser::help() const {
    const Impl& defs = *m_impl;
    const int width = terminal_width();
    std::string out;

    if (!defs.description.empty()) {
        for (const std::string& line : wrap(defs.description, static_cast<std::size_t>(width)))
            out += line + "\n";
        out += "\n";
    }

    out += usage() + "\n";

    // Positional arguments
    std::vector<std::pair<std::string, std::string>> positional_rows;
    for (const Flag* flag : defs.positionals) {
        const Flag::Impl& f = *flag->m_impl;
        if (f.hidden) continue;
        std::string right = f.help;
        const std::string notes = notes_for(*flag);
        if (!notes.empty()) right += (right.empty() ? "" : " ") + notes;
        positional_rows.emplace_back(f.name, right);
    }
    if (!positional_rows.empty()) {
        out += "\nPositional arguments:\n";
        out += render_rows(positional_rows, width);
    }

    // Options
    std::vector<std::pair<std::string, std::string>> option_rows;
    for (const auto& holder : defs.flags) {
        const Flag::Impl& f = *holder->m_impl;
        if (f.positional || f.hidden) continue;
        std::string right = f.help;
        const std::string notes = notes_for(*holder);
        if (!notes.empty()) right += (right.empty() ? "" : " ") + notes;
        option_rows.emplace_back(option_left(*holder), right);
    }
    if (!option_rows.empty()) {
        out += "\nOptions:\n";
        out += render_rows(option_rows, width);
    }

    if (!defs.epilog.empty()) {
        out += "\n";
        for (const std::string& line : wrap(defs.epilog, static_cast<std::size_t>(width)))
            out += line + "\n";
    }
    return out;
}

}  // namespace flageasy
