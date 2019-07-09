//
// Created by thoma on 17/06/2019.
//

#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <optional>
#include <variant>
#include "cparse.h"

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

        template<uint32_t I = 0>
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

    [[nodiscard]] std::optional<Variable> operator[](const std::string& key) const noexcept
    {
        const auto iter = map.find(key);
        if(iter != end(map)) return {iter->second};
        else return {};
    }

    std::map<std::string, Variable> map;
};

template<typename Section>
class SectionMap
{
public:
    SectionMap(const std::string& path) { read(path); }
    SectionMap(const char* path) { read(path); }

    [[nodiscard]] const Section& operator[](const std::string& key) const
    {
        const auto iter = map.find(key);
        if(iter != end(map)) return iter->second;
        else throw std::runtime_error("could not find section key\n");
    }

private:
    void read(const char* path)
    {
        const std::string data = cparse(path);
        auto current = begin(data);
        auto section = end(map);

        // seems scary but enough end of file checks are done, trust me
        while(true)
        {
            if(*current == '[')
            {
                // find the closing brace and create a new section.
                const auto iter = findCharOnLine(current, end(data), ']');
                if(not iter.has_value()) throw std::runtime_error("did not close ending brace before end of file or line");
                std::string name(current + 1, iter.value());
                section = map.try_emplace(name, Section{}).first;

                // skip to the next line, if there is none we can return
                auto next = skipWhiteSpace(iter.value()+1, end(data));
                if(next.has_value()) current = next.value();
                else return;
            }
            else if(*current == '#')
            {
                auto next = findCharOnLine(current, end(data));
                if(next.has_value()) current = next.value();
                else return;
            }
            else if(section != end(map))
            {
                // find the = delimiter
                const auto iter = findCharOnLine(current, end(data), '=');
                if(not iter.has_value()) throw std::runtime_error("could not find = on line");
                auto nameIter = iter.value() - 1;
                auto varIter  = iter.value() + 1;

                // search until no more whitespaces
                while(*nameIter == ' ') nameIter--;
                while(varIter != end(data) and *varIter == ' ') varIter++;
                auto endVarIter = findCharOnLine(varIter, end(data));

                // construct a new entry in the section
                auto result = section->second.map.try_emplace(std::string(current, nameIter+1), Section::Variable::read(varIter, endVarIter.value_or(end(data)) ));
                if(not result.second) throw std::runtime_error("duplicate value");
                if(not endVarIter.has_value()) return;

                // skip to next line, if there is none, we can return
                auto next = skipWhiteSpace(endVarIter.value()+1, end(data));
                if(next.has_value()) current = next.value();
                else return;
            }
            else
            {
                throw std::runtime_error("assigning elements without section");
            }
        }
    }

    std::map<std::string, Section> map;
};

using Section = VarMap<std::string, double, int, bool>;
using Configuration = SectionMap<Section>;
