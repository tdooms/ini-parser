//============================================================================
// @name        : typeParsers.h
// @author      : Thomas Dooms
// @date        : 7/17/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once

#include <string>
#include <optional>
#include <algorithm>
#include "util.h"

// REQUIRE: end > begin
// REQUIRE: all memory until end is valid

template<typename T>
struct false_type : std::false_type { };

template<typename T>
std::optional<T> parse([[maybe_unused]] std::string::const_iterator begin, [[maybe_unused]] std::string::const_iterator end) noexcept
{
    // this always throws a compile time error, but we have to trick the compiler because assert(false) doesn't work.
    static_assert(false_type<T>::value, "could not find parser for a certain type, please check if every type has a corresponding parser");
    return {};
}

template<>
std::optional<int> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    bool negative = false;
    int result = 0;

    if(*begin == '+') begin++;
    else if(*begin == '-'){ begin++; negative = true; }
    else if(*begin < '0' or *begin > '9') return {};

    while(begin != end and *begin >= '0' and *begin <= '9')
    {
        result *= 10;
        result += *begin - '0';
        begin++;
    }
    return negative ? -result : result;
}

template<>
std::optional<double> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    bool negative = false;
    double result = 0;

    if(*begin == '+') begin++;
    else if(*begin == '-'){ begin++; negative = true; }
    else if(*begin < '0' or *begin > '9') return {};

    while(begin != end and *begin >= '0' and *begin <= '9')
    {
        result *= 10;
        result += *begin - '0';
        begin++;
    }
    if(begin == end or *begin != '.') return {};
    begin++;

    double pow = 0.1;

    while(begin != end and *begin >= '0' and *begin <= '9')
    {
        result += (*begin - '0') * pow;
        pow /= 10;
        begin++;
    }
    return negative ? -result : result;
}

template<>
std::optional<std::string> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    if(*begin != '\"') return {};

    auto iter = findCharOnLine(begin + 1, end, '\"');
    if(iter.has_value()) return std::string(begin+1, iter.value());
    else return {};
}

template<>
std::optional<bool> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    constexpr char trueChars [] = "true" ;
    constexpr char falseChars[] = "false";

    const auto size = end - begin;
    if(size > 4 and std::equal(trueChars , trueChars+4 , begin)) return true;
    if(size > 5 and std::equal(falseChars, falseChars+5, begin)) return false;
    else return {};
}

template<>
std::optional<std::vector<double>> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    // requires a double parser, which is provided by default.
    if(*begin != '(') return {};

    std::vector<double> result;
    result.reserve(3); // 3 is probably the most common length

    while(*begin != ')')
    {
        // we skip the whitespace before the value
        begin++;
        const auto start = skipWhiteSpace(begin, end);
        if(start.has_value()) begin = start.value();
        else return {};

        // we try to find the end of the value either by finding ',' or ')'
        auto next = findCharOnLine(begin, end, ',');
        if(not next.has_value()) next = findCharOnLine(begin, end, ')');
        if(not next.has_value()) return {};
        auto valueEnd = next.value();

        // we decrement to skip whitespace
        --valueEnd;
        while(*valueEnd == ' ') --valueEnd;

        const auto value = parse<double>(begin, valueEnd);
        if(not value.has_value()) return {};
        result.push_back(*value);

        begin = next.value();
    }
    return result;
}
