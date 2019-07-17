//============================================================================
// @name        : configuration.h
// @author      : Thomas Dooms
// @date        : 7/17/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once

#include <iostream>
#include <optional>
#include <variant>

#include "type-parsers.h"
#include "quick-types.h"

template<typename... Types>
class VarMap
{
public:
    class Variable
    {
    public:
        template<typename T>
        Variable(const T& data) : var(data)
        {
            static_assert(std::disjunction<std::is_same<T, Types>...>(), "type not supported");
        }

        template<typename T>
        [[nodiscard]] operator T() const
        {
            static_assert(std::disjunction<std::is_same<T, Types>...>(), "type not supported");
            return std::get<T>(var);
        }

        template<size_t I = 0>
        [[nodiscard]] static Variable read([[maybe_unused]] std::string::const_iterator begin, [[maybe_unused]] std::string::const_iterator end)
        {
            if constexpr (I == sizeof...(Types)) throw std::runtime_error("no suitable value could be parsed from : " + std::string(begin, end+1));
            else
            {
                auto result = parse<typename std::tuple_element<I, typename std::tuple<Types...>>::type>(begin, end);
                if(result.has_value()) return result.value();
                return read<I+1>(begin, end);
            }
        }

    private:
        std::variant<Types...> var;
    };

    template<typename T>
    [[nodiscard]] std::optional<Variable> operator[](const T& key) const noexcept
    {
        static_assert(std::is_convertible<T, dot::short_string>(), "key type not convertible to string");
        const auto iter = map.find(key);
        if(iter != map.end()) return {iter->second};
        else return {};
    }

    dot::quick_map<Variable> map;
};

template<typename Section>
class SectionMap
{
public:
    explicit SectionMap(const std::string& path) { read(path); }
    explicit SectionMap(const char* path) { read(path); }

    template<typename T>
    [[nodiscard]] const Section& operator[](const T& key) const
    {
        static_assert(std::is_convertible<T, dot::short_string>(), "type not convertible to string");
        const auto iter = map.find(key);
        if(iter != map.end()) return iter->second;
        else throw std::runtime_error("could not find section key\n");
    }

private:
    void read(const char* path)
    {
        const std::string data = cparse(path);
        auto current = begin(data);
        auto section = end(map);
        size_t line = 1;

        // seems scary but enough end of file checks are done, trust me
        while(true)
        {
            // create new section
            if(*current == '[')
            {
                const auto iter = findCharOnLine(current, end(data), ']');
                if(not iter.has_value()) throw std::runtime_error("did not ']' before end of file or line on line: " + std::to_string(line));
                section = map.try_emplace({current + 1, iter.value()}, Section{}).first;
                current = iter.value();
            }
            // skip over comment
            else if(*current == '#')
            {
            }
            // throw error for whitespace
            else if(isWhiteSpace(*current))
            {
                const auto next = skipWhiteSpace(current, end(data));
                if(next.has_value()) current = next.value();
                else return;

                if(*current == '#') {}
                else throw std::runtime_error("whitespace at start of line: " + std::to_string(line));
            }
            else if(*current == '\n' or *current == '\r')
            {
                skipLine(current, end(data));
            }
            // if there is an active section, every line is an entry
            else if(section != end(map))
            {
                const auto iter = findCharOnLine(current, end(data), '=');
                if(not iter.has_value()) throw std::runtime_error("could not find = on line: " + std::to_string(line));

                const auto nameIter   = skipWhiteSpaceReverse(iter.value() - 1);
                const auto varIter    = skipWhiteSpace(iter.value() + 1, end(data));
                if(not varIter.has_value()) throw std::runtime_error("no value after = on line: " + std::to_string(line));

                const auto endVarIter = findCharOnLine(varIter.value(), end(data), '\n');

                // construct a new entry in the section
                auto result = section->second.map.try_emplace({current, nameIter + 1}, Section::Variable::read(varIter.value(), endVarIter.value_or(end(data))));
                if(not result.second) throw std::runtime_error("duplicate value on line: " + std::to_string(line));
                if(not endVarIter.has_value()) return;
            }
            else
            {
                throw std::runtime_error("assigning elements without section on line: " + std::to_string(line));
            }

            line++;
            const auto next = skipLine(current, end(data));
            if(next.has_value()) current = next.value();
            else return;
        }
    }

    dot::quick_map<Section> map;
};

using Section = VarMap<std::string, double, int, bool, std::vector<double>>;
using Configuration = SectionMap<Section>;



