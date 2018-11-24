#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

template<class T> using Maybe = std::optional<T>;

using Include = std::string;
using Comment = std::string;
using Identifier = std::string;

struct Plugin {
    std::string name;
    std::string type;
};

struct Vrscene {
    std::vector<Include> includes;
    std::vector<Comment> comments;
    std::vector<Plugin> plugins;
};

inline static void trim_leading_whitespace(std::string_view& string) {
    while(std::isspace(string.front())) string.remove_prefix(1);
}

inline static void trim_trailing_whitespace(std::string_view& string) {
    while(std::isspace(string.back())) string.remove_suffix(1);
}

inline static bool match(std::string_view string, std::string_view token) {
    return string.compare(0, token.size(), token) == 0;
}

inline static bool try_consume(std::string_view& string, std::string_view token) {
    if (!match(string, token)) return false;
    string.remove_prefix(token.size());
    return true;
}

inline static bool is_space(char ch){
    return std::isspace(static_cast<unsigned char>(ch));
}

static Maybe<std::string_view> qouted_string(std::string_view& string) {
    if (string[0] != '"') return {};
    auto end = string.find('"', 1);
    auto res = string.substr(1, end - 1);
    string.remove_prefix(end + 1);
    return res;
}

static Maybe<Include> parse_include(std::string_view& string) {
    trim_leading_whitespace(string);
    if (!try_consume(string, "#include")){
        return {};
    }
    trim_leading_whitespace(string);
    auto file = qouted_string(string);
    return file ? std::make_optional(Include(*file)) : std::nullopt;
}

static Maybe<Comment> parse_comment(std::string_view& source) {
    trim_leading_whitespace(source);
    if (!match(source, "//")) return {};

    auto eol = source.find('\n');
    auto comment = source.substr(0, eol);
    trim_trailing_whitespace(comment);
    source.remove_prefix(eol);
    return Comment(comment);
}


static Maybe<Identifier> parse_identifier(std::string_view& src) {
    trim_leading_whitespace(src);

    if(!std::isalpha(src.front())) return {};

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

static Maybe<Plugin> parse_plugin(std::string_view& src) {
    auto type = parse_identifier(src);
    auto name = parse_identifier(src);
    if (!(type && name)) return {};
    auto begin = src.find('{');
    auto end = src.find('}');
    src.remove_prefix(end + 1);

    return Plugin{*name, *type};
}

static Maybe<Vrscene> parse_vrscene(const std::string_view& src) {
    auto source = src;
    Vrscene scene;

    while(!source.empty()) {
        if(auto comment = parse_comment(source); comment) {
            scene.comments.push_back(*comment);
            //std::cout << "PARSED COMMENT: " << *comment << std::endl;
        } else if (auto import = parse_include(source); import) {
            scene.includes.push_back(*import);
            //std::cout << "PARSED IMPORT: " << *import << std::endl;
        } else if(auto plugin = parse_plugin(source); plugin) {
            scene.plugins.push_back(*plugin);
            //std::cout << "PARSED PLUGIN: " << plugin->name << std::endl;
        } else {
            return {};
        }
        trim_leading_whitespace(source);
    }

    return scene;
}
