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
#include <string>
#include <optional>

struct ParseUtils
{
    static bool isWhiteSpace(char c)
    {
        return c == ' ' or c == '\t';
    }

    static const std::string readToString(const std::string& path)
    {
        auto file = fopen(path.c_str(), "rb");
        fseek(file, 0, SEEK_END);
        auto size = static_cast<size_t>(ftell(file));
        fseek(file, 0, SEEK_SET);

        std::string string(size, 0);
        fread(string.data(), 1, size, file);
        fclose(file);
        return string;
    }

    static std::optional<std::string::const_iterator> findCharOnLine(std::string::const_iterator begin, std::string::const_iterator end, char c) noexcept
    {
        for(; begin != end; begin++)
        {
            if(*begin == c) return begin;
            else if(*begin == '\n') return {};
        }
        return {};
    }

    static std::optional<std::string::const_iterator> skipLine(std::string::const_iterator begin, std::string::const_iterator end) noexcept
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

    static std::optional<std::string::const_iterator> skipWhiteSpace(std::string::const_iterator begin, std::string::const_iterator end) noexcept
    {
        for(; begin != end; begin++)
        {
            if(not isWhiteSpace(*begin)) return begin;
        }
        return {};
    }

    static std::string::const_iterator skipWhiteSpaceReverse(std::string::const_iterator begin) noexcept
    {
        while(isWhiteSpace(*begin)) begin--;
        return begin++;
    }
};







