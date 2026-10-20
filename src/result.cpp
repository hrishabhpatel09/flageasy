// flageasy — ParseResult implementation.

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "internal.hpp"

namespace flageasy {

ParseResult::ParseResult() : m_impl(std::make_unique<Impl>()) {}
ParseResult::~ParseResult() = default;

ParseResult::ParseResult(const ParseResult& other)
    : m_impl(std::make_unique<Impl>(*other.m_impl)) {}

ParseResult& ParseResult::operator=(const ParseResult& other) {
    if (this != &other) *m_impl = *other.m_impl;
    return *this;
}

ParseResult::ParseResult(ParseResult&& other) noexcept = default;
ParseResult& ParseResult::operator=(ParseResult&& other) noexcept = default;

// --- status -------------------------------------------------------------------

bool ParseResult::ok() const noexcept { return m_impl->ok; }

const ParseError& ParseResult::error() const noexcept { return m_impl->error; }

const std::string& ParseResult::error_message() const noexcept {
    return m_impl->error.message;
}

bool ParseResult::help_requested() const noexcept { return m_impl->help; }

bool ParseResult::version_requested() const noexcept { return m_impl->version; }

// --- raw access ------------------------------------------------------------------

bool ParseResult::has(const std::string& name) const noexcept {
    return m_impl->index.count(name) != 0u;
}

bool ParseResult::is_set(const std::string& name) const noexcept {
    const auto it = m_impl->index.find(name);
    return it != m_impl->index.end() && m_impl->flags[it->second].is_set;
}

const std::vector<std::string>& ParseResult::raw(const std::string& name) const {
    const auto it = m_impl->index.find(name);
    if (it == m_impl->index.end())
        throw std::out_of_range("flageasy: unknown flag '" + name + "'");
    return m_impl->flags[it->second].values;
}

std::optional<std::string> ParseResult::value(const std::string& name) const {
    const auto it = m_impl->index.find(name);
    if (it == m_impl->index.end()) return std::nullopt;
    const std::vector<std::string>& values = m_impl->flags[it->second].values;
    if (values.empty()) return std::nullopt;
    return values.front();
}

// --- enumeration -------------------------------------------------------------------

const std::vector<ParsedFlag>& ParseResult::flags() const noexcept {
    return m_impl->flags;
}

const std::vector<std::string>& ParseResult::positionals() const noexcept {
    return m_impl->positionals;
}

const std::vector<std::string>& ParseResult::unknown() const noexcept {
    return m_impl->unknown;
}

std::vector<std::string> ParseResult::names() const {
    std::vector<std::string> out;
    out.reserve(m_impl->flags.size());
    for (const ParsedFlag& flag : m_impl->flags) out.push_back(flag.name);
    return out;
}

std::string ParseResult::to_string() const {
    std::string out;
    for (const ParsedFlag& flag : m_impl->flags) {
        if (!out.empty()) out += ", ";
        out += flag.name;
        out += '=';
        if (flag.values.empty()) {
            out += "<unset>";
        } else {
            for (std::size_t k = 0; k < flag.values.size(); ++k) {
                if (k != 0) out += '|';
                out += flag.values[k];
            }
            if (!flag.is_set) out += "(default)";
        }
    }
    if (!m_impl->unknown.empty()) {
        out += " (unknown:";
        for (const std::string& token : m_impl->unknown) {
            out += ' ';
            out += token;
        }
        out += ')';
    }
    return out;
}

}  // namespace flageasy
