//============================================================================
// @name        : cparse.h
// @author      : Thomas Dooms
// @date        : 6/23/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <optional>
#include <algorithm>

static const std::string cparse(const char* path)
{
    auto file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    auto size = static_cast<size_t>(ftell(file));
    fseek(file, 0, SEEK_SET);

    std::string string(size, 0);
    fread(string.data(), 1, size, file);
    fclose(file);
    return string;
}

static std::optional<std::string::const_iterator> findCharOnLine(std::string::const_iterator begin, std::string::const_iterator end, char c = '\n') noexcept
{
    for(auto current = begin; current != end; current++)
    {
        // character found
        if(*current == c) return current;
        // character not found before end of line
        else if(*current == '\n') return {};
    }
    //character not found before end of string
    return {};
}

static std::optional<std::string::const_iterator> skipWhiteSpace(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    for(auto current = begin; current != end; current++)
    {
        if(*current == ' ' or *current == '\n' or *current == '\t' or *current == '\r') current++;
        // all white space was skipped
        else return current;
    }
    // nothing else but whitespace before end of file
    return {};
}

// require: end > begin
// require: all memory until end is valid

template<typename T>
struct temp : std::false_type { };

template<typename T>
std::optional<T> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    static_assert(temp<T>::value, "could not find parser for a certain type\n");
    return {};
}

template<>
std::optional<int> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    auto current = begin;
    int result = 0;
    bool negative = false;

    if(*current == '+') current++;
    else if(*current == '-'){ current++; negative = true; }
    else if(*current < '0' or *current > '9') return {};

    while(current != end and *current >= '0' and *current <= '9')
    {
        result *= 10;
        result += *current - '0';
        current++;
    }
    return (negative) ? -result : result;
}

template<>
std::optional<double> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    auto current = begin;
    double result = 0;
    bool negative = false;

    if(*current == '+') current++;
    else if(*current == '-'){ current++; negative = true; }
    else if(*current < '0' or *current > '9') return {};

    while(current != end and *current >= '0' and *current <= '9')
    {
        result *= 10;
        result += *current - '0';
        current++;
    }
    if(current == end or *current != '.') return {};
    current++;

    double pow = 0.1;

    while(current != end and *current >= '0' and *current <= '9')
    {
        result += (*current - '0') * pow;
        pow /= 10;
        current++;
    }
    return (negative) ? -result : result;
}

template<>
std::optional<std::string> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    const auto current = begin;
    if(*current != '\"') return {};

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
