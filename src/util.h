//============================================================================
// @name        : util.h
// @author      : Thomas Dooms
// @date        : 7/17/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <optional>

bool isWhiteSpace(char c)
{
    return c == ' ' or c == '\t' or c == '\n' or c == '\r';
}

const std::string cparse(const char* path)
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

std::optional<std::string::const_iterator> findCharOnLine(std::string::const_iterator begin, std::string::const_iterator end, char c) noexcept
{
    for(; begin != end; begin++)
    {
        if(*begin == c) return begin;
        else if(*begin == '\n') return {};
    }
    return {};
}

std::optional<std::string::const_iterator> skipLine(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    for(; begin != end; begin++)
    {
        if(*begin == '\n')
        {
            if(++begin == end) return {};
            return begin;
        }
    }
    return {};
}

std::optional<std::string::const_iterator> skipWhiteSpace(std::string::const_iterator begin, std::string::const_iterator end) noexcept
{
    for(; begin != end; begin++)
    {
        if(not isWhiteSpace(*begin)) return begin;
    }
    return {};
}

std::string::const_iterator skipWhiteSpaceReverse(std::string::const_iterator begin) noexcept
{
    while(isWhiteSpace(*begin)) begin--;
    return begin++;
}