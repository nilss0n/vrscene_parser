#pragma once
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace vrp {

    template<class T> using Maybe = std::optional<T>;

    struct Include : public std::string {
        using std::string::string;
    };

    struct Comment : public std::string {
        using std::string::string;
    };

    using Identifier = std::string;

    struct AttributeSelector {
        Identifier plugin;
        Identifier attribute;
    };

    struct Function;

    using Value = std::variant<AttributeSelector, Function, Identifier, int64_t, double_t, std::string_view>;

    struct Function {
        std::string name;
        std::vector<Value> arguments;

        Value operator[](size_t idx) const {
            return arguments[idx];
        }
    };

    struct Plugin {
        std::string name;
        std::string type;
        std::vector<std::pair<Identifier, Value>> attributes;
    };

    struct Vrscene {
        std::vector<Include> includes;
        std::vector<Comment> comments;
        std::vector<Plugin> plugins;
        std::string source;
    };

    inline void trim_leading_whitespace(std::string_view& string) noexcept {
        while(!string.empty() && std::isspace(string.front())) string.remove_prefix(1);
    }

    inline void trim_trailing_whitespace(std::string_view& string) noexcept {
        while(std::isspace(string.back())) string.remove_suffix(1);
    }

    inline bool match(std::string_view string, std::string_view token) noexcept {
        return string.compare(0, token.size(), token) == 0;
    }

    inline bool try_consume(std::string_view& string, std::string_view token, bool trim = true) noexcept {
        if(trim) trim_leading_whitespace(string);
        if(!match(string, token)) return false;
        string.remove_prefix(token.size());
        return true;
    }

    inline bool is_space(char ch) noexcept {
        return std::isspace(static_cast<unsigned char>(ch));
    }

    inline Maybe<std::string_view> parse_qouted_string(std::string_view& src) {
        if(!try_consume(src, "\"")) return {};
        auto end = src.find('"');
        auto val = src.substr(0, end);
        src.remove_prefix(end + 1);
        return val;
    }

    inline Maybe<Include> parse_include(std::string_view& string) {
        if(!try_consume(string, "#include")){
            return {};
        }
        trim_leading_whitespace(string);
        auto file = parse_qouted_string(string);
        return file ? std::make_optional(Include(*file)) : std::nullopt;
    }

    inline Maybe<Comment> parse_comment(std::string_view& source) {
        trim_leading_whitespace(source);
        if(!match(source, "//")) return {};

        auto eol = source.find('\n');
        auto comment = source.substr(0, eol);
        trim_trailing_whitespace(comment);
        source.remove_prefix(eol);
        return Comment(comment);
    }

    inline auto is_valid_identifier_start(char c) noexcept {
        return std::isalpha(c) || static_cast<uint8_t>(c) >= 0xC0 || c == '_';
    }

    inline Maybe<Identifier> parse_identifier(std::string_view& src) {
        trim_leading_whitespace(src);

        if(!is_valid_identifier_start(src.front())) return {};

        const auto is_identifier_end = [](char c) {
            static const std::string identifier_breaks = "=;{}:,()";
            return is_space(c) || identifier_breaks.find(c) != std::string::npos;
        };

        auto end = 0;
        while(!is_identifier_end(src[++end]));
        auto result = Identifier(src.substr(0, end));
        src.remove_prefix(end);
        return result;
    }

    inline Maybe<Value> parse_number(std::string_view& src) {
        char *ds = nullptr, *is=nullptr;
        const auto pos = &src[0];
        auto d = Value(std::strtod(pos, &ds));
        auto i = Value(std::strtoll(pos, &is, 10));
        if(ds == pos && is == pos) return {};
        src.remove_prefix((ds > is ? ds : is) - pos);
        return ds > is ? std::make_optional(d) : std::make_optional(i);
    }

    inline Maybe<Value> parse_attribute_selector(std::string_view& source) {
        auto src = source;
        auto p = parse_identifier(src);
        auto dots = try_consume(src, "::", false);
        auto a = parse_identifier(src);

        if(!(p && dots && a)) return {};

        source = src;
        return AttributeSelector{*p, *a};
    }

    Maybe<Value> parse_function(std::string_view& src);

    inline Maybe<Value> parse_value(std::string_view& src) {
        if(auto n = parse_number(src)) {
            return *n;
        } else if(auto s = parse_qouted_string(src)) {
            return *s;
        } else if(auto f = parse_function(src)) {
            return *f;
        } else if(auto as = parse_attribute_selector(src)) {
            return *as;
        }else if(auto i = parse_identifier(src)) {
            return *i;
        } else {
            return {};
        }
    }

    inline Maybe<Value> parse_function(std::string_view& source) {
        auto src = source;
        auto n = parse_identifier(src);
        if(!n) return {};
        Function f;
        f.name = *n;
        if(!try_consume(src, "(")) return {};

        bool first = true;
        while(!try_consume(src, ")")) {
            if(!first && !try_consume(src, ",")) return {};
            first = false;
            auto v = parse_value(src);
            if(!v) return {};
            f.arguments.push_back(*v);
        }

        source = src;
        return f;
    }

    inline Maybe<Plugin> parse_plugin(std::string_view& src) {
        auto type = parse_identifier(src);
        auto name = parse_identifier(src);
        if(!(type && name)) return {};

        auto begin = src.find('{');
        src.remove_prefix(begin + 1);
        Plugin p;
        p.name = *name;
        p.type = *type;

        while(!try_consume(src, "}")) {
            if(auto comment = parse_comment(src); comment) continue;
            auto id = parse_identifier(src);
            if(!id) return {};
            if(!try_consume(src, "=")) return {};
            auto val = parse_value(src);
            if(!val) return {};
            if(!try_consume(src, ";")) return {};
            p.attributes.push_back(std::make_pair(*id, *val));
        }

        return p;
    }

    inline Maybe<Vrscene> parse_vrscene(const std::string& src) {
        std::string_view source = src;
        Vrscene scene;
        scene.source = std::move(src);

        while(!source.empty()) {
            if(auto comment = parse_comment(source); comment) {
                scene.comments.push_back(*comment);
            } else if(auto import = parse_include(source); import) {
                scene.includes.push_back(*import);
            } else if(auto plugin = parse_plugin(source); plugin) {
                scene.plugins.push_back(*plugin);
            } else {
                return {};
            }
            trim_leading_whitespace(source);
        }

        return scene;
    }

    inline std::ostream& operator<<(std::ostream& os, const Comment& c) {
        return os << std::string_view(c) << '\n';
    }

    inline std::ostream& operator<<(std::ostream& os, const Include& inc) {
        return os << "#include \"" << std::string_view(inc) << "\"\n";
    }

    inline std::ostream& operator<<(std::ostream& os, const Value& v) {
        if(auto str = std::get_if<std::string_view>(&v)) {
            return os << '"' << *str << '"';
        } else if(auto as = std::get_if<AttributeSelector>(&v)) {
            return os << as->plugin << "::" << as->attribute;
        } else if(auto f = std::get_if<Function>(&v)) {
            os << f->name << '(';
            auto first = true;
            for(auto&& a: f->arguments) {
                if(!first) os << ',';
                first = false;
                os << a;
            }
            return os << ')';
        }
        std::visit([&os](auto const& e){ os << e; }, v);
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const Plugin& p) {
        os << p.type << " " << p.name << " {\n";
        for(auto&& a : p.attributes) {
            os << "  " << a.first << "=" << a.second << ";\n";
        }
        return os << "}";
    }

} // namespace vrp
