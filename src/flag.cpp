// flageasy — Flag definition object implementation.

#include "internal.hpp"

namespace flageasy {

Flag::Flag(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
Flag::~Flag() = default;

// --- introspection ----------------------------------------------------------

const std::string& Flag::name() const noexcept { return m_impl->name; }
char Flag::short_name() const noexcept { return m_impl->short_name; }
ValueType Flag::type() const noexcept { return m_impl->type; }
bool Flag::is_list() const noexcept { return m_impl->list; }
bool Flag::is_positional() const noexcept { return m_impl->positional; }
bool Flag::is_rest() const noexcept { return m_impl->rest; }
bool Flag::is_required() const noexcept { return m_impl->required; }
bool Flag::is_hidden() const noexcept { return m_impl->hidden; }
bool Flag::takes_value() const noexcept { return m_impl->type != ValueType::Switch; }
const std::string& Flag::help_text() const noexcept { return m_impl->help; }

// --- fluent configuration -----------------------------------------------------

Flag& Flag::help(std::string text) {
    m_impl->help = std::move(text);
    return *this;
}

Flag& Flag::metavar(std::string text) {
    m_impl->metavar = std::move(text);
    return *this;
}

Flag& Flag::required(bool on) {
    m_impl->required = on;
    return *this;
}

Flag& Flag::hidden(bool on) {
    m_impl->hidden = on;
    return *this;
}

Flag& Flag::default_value(bool value) {
    m_impl->has_default = true;
    m_impl->defaults.assign(1, value ? "true" : "false");
    return *this;
}

Flag& Flag::default_value(const char* value) {
    return default_value(std::string(value != nullptr ? value : ""));
}

Flag& Flag::default_value(std::string value) {
    m_impl->has_default = true;
    m_impl->defaults.assign(1, std::move(value));
    return *this;
}

Flag& Flag::default_value_signed(long long value) {
    m_impl->has_default = true;
    m_impl->defaults.assign(1, std::to_string(value));
    return *this;
}

Flag& Flag::default_value_real(double value) {
    m_impl->has_default = true;
    m_impl->defaults.assign(1, internal::format_number(value));
    return *this;
}

Flag& Flag::default_values(std::vector<std::string> values) {
    m_impl->has_default = true;
    m_impl->defaults = std::move(values);
    return *this;
}

Flag& Flag::range(double low, double high) {
    m_impl->has_range = true;
    m_impl->range_lo = low;
    m_impl->range_hi = high;
    return *this;
}

Flag& Flag::choices(std::vector<std::string> allowed) {
    m_impl->choices = std::move(allowed);
    return *this;
}

Flag& Flag::validate(std::function<bool(const std::string&, std::string&)> fn) {
    m_impl->validator = std::move(fn);
    return *this;
}

}  // namespace flageasy
