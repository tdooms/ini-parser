//============================================================================
// @name        : test.h
// @author      : Thomas Dooms
// @date        : 8/10/19
// @version     :
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description :
//============================================================================


#pragma once

#include <variant>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <unordered_map>
#include <iomanip>

template<typename T>
struct false_type : std::false_type {};

template<typename T>
std::ofstream& operator<<(std::ofstream& ofstream, const std::vector<T>& vec) noexcept
{
    ofstream << '(';
    for(size_t i = 0; i < vec.size()-1; i++) ofstream << vec[i] << ", ";
    ofstream << vec.back() << ')';
    return ofstream;
}

namespace ParseUtils
{
    bool isWhiteSpace(char c)
    {
        return c == ' ' or c == '\t';
    }

    const std::string readToString(const std::string &path)
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

    std::optional<std::string::const_iterator> findCharOnLine(std::string::const_iterator begin, std::string::const_iterator end, char c) noexcept
    {
        for (; begin != end; begin++)
        {
            if (*begin == c) return begin;
            else if (*begin == '\n') return {};
        }
        return {};
    }

    std::optional<std::string::const_iterator> skipLine(std::string::const_iterator begin, std::string::const_iterator end) noexcept
    {
        for (; begin != end; begin++)
        {
            if (*begin == '\n')
            {
                if (++begin == end) return {};
                return begin;
            }
        }
        return {};
    }

    std::optional<std::string::const_iterator> skipWhiteSpace(std::string::const_iterator begin, std::string::const_iterator end) noexcept
    {
        for (; begin != end; begin++)
        {
            if (not isWhiteSpace(*begin)) return begin;
        }
        return {};
    }

    std::string::const_iterator skipWhiteSpaceReverse(std::string::const_iterator begin) noexcept
    {
        while (isWhiteSpace(*begin)) begin--;
        return begin++;
    }


    template<typename T>
    std::optional<T> parse(std::string::const_iterator, std::string::const_iterator) noexcept
    {
        static_assert(true, "SFINAE stuff");
        return {};
    }

    template<>
    std::optional<long> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept
    {
        bool negative = false;
        long result = 0;

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

        auto iter = ParseUtils::findCharOnLine(begin + 1, end, '\"');
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

    template<typename T>
    std::optional<std::vector<T>> parseVector(std::string::const_iterator begin, std::string::const_iterator end) noexcept
    {
        // requires a double parser, which is provided by default.
        if(*begin != '(') return {};

        std::vector<T> result;
        result.reserve(3); // 3 is probably the most common length

        while(*begin != ')')
        {
            // we skip the whitespace before the value
            begin++;
            const auto start = ParseUtils::skipWhiteSpace(begin, end);
            if(start.has_value()) begin = start.value();
            else return {};

            // we try to find the end of the value either by finding ',' or ')'
            auto next = ParseUtils::findCharOnLine(begin, end, ',');
            if(not next.has_value()) next = ParseUtils::findCharOnLine(begin, end, ')');
            if(not next.has_value()) return {};
            auto valueEnd = next.value();

            // we decrement to skip whitespace
            --valueEnd;
            while(*valueEnd == ' ') --valueEnd;

            const auto value = parse<T>(begin, valueEnd);
            if(not value.has_value()) return {};
            result.push_back(*value);

            begin = next.value();
        }
        return result;
    }
}

namespace dot { class ini; }
std::ofstream& operator<<(std::ofstream&, const dot::ini&);

namespace dot
{

class inivalue
{
public:
    friend class entry;
    friend class ini;

    inivalue() = default;

    template<typename T>
    inivalue(const T& value)
    {
        if      constexpr (std::is_integral_v      <T>) var = static_cast<long>(value);
        else if constexpr (std::is_floating_point_v<T>) var = static_cast<double>(value);
        else if constexpr (std::is_convertible_v<T, std::string>) var = std::string(value);
        else static_assert(false_type<T>::value, "type not supported");
    }

    template<typename T>
    inivalue(const std::vector<T>& value)
    {
        if      constexpr (std::is_integral_v      <T>) var = static_cast<std::vector<long>>(value);
        else if constexpr (std::is_floating_point_v<T>) var = static_cast<std::vector<double>>(value);
        else static_assert(false_type<T>::value, "vector type not supported");
    }

    template<typename T>
    [[nodiscard]] operator T() const noexcept
    {
        if      constexpr (std::is_integral_v      <T>) return static_cast<T>(std::get<long>(var));
        else if constexpr (std::is_floating_point_v<T>) return static_cast<T>(std::get<double>(var));
        else if constexpr (std::is_convertible_v<T, std::string>) return std::get<std::string>(var);
        else static_assert(false_type<T>::value, "type not supported");
    }

    template<typename T>
    [[nodiscard]] operator std::vector<T>() const noexcept
    {
        if constexpr(std::is_integral_v<T>)
        {
            auto temp = std::get<std::vector<long>>(var);
            return std::vector<T>{begin(temp), end(temp)};
        }
        else if constexpr(std::is_floating_point_v<T>)
        {
            auto temp = std::get<std::vector<double>>(var);
            return std::vector<T>{begin(temp), end(temp)};
        }
        else static_assert("vector type not supported");
    }

private:
    std::variant<std::monostate, double, long, std::string, std::vector<double>, std::vector<long>> var;
};


class entry
{
public:

    entry() = default;

    entry(std::string::const_iterator begin, std::string::const_iterator end);

    [[nodiscard]] const inivalue& value() const;

    [[nodiscard]] bool has_value() const noexcept;

    [[nodiscard]] const auto& variant() const noexcept { return var.var; }

    template<typename... Tp>
    [[nodiscard]] inivalue value_or(Tp&&... values) const noexcept;

    template<typename... Tp>
    void write(Tp&&... values);

private:
    inivalue var;
};

class section
{
public:
    friend class ini;

    template<typename T>
    [[nodiscard]] const entry& operator[](const T& key) const noexcept;

    template<typename T>
    [[nodiscard]] entry& operator[](const T& key) noexcept;

    [[nodiscard]] auto begin() const noexcept { return map.begin(); }
    [[nodiscard]] auto end()   const noexcept { return map.end(); }

    [[nodiscard]] auto begin() noexcept { return map.begin(); }
    [[nodiscard]] auto end()   noexcept { return map.end(); }

    [[nodiscard]] auto size()  const noexcept { return map.size(); }
    [[nodiscard]] auto empty() const noexcept { return map.empty(); }

private:
    std::unordered_map<std::string, entry> map;
    static inline entry newEntry;
};

class ini
{
public:

    ini() = default;

    explicit ini(const char* path);

    friend std::ofstream& ::operator<<(std::ofstream& ofstream, const dot::ini& ini);

    template<typename T>
    [[nodiscard]] const section& operator[](const T& key) const;

    template<typename T>
    [[nodiscard]] section& operator[](const T& key) noexcept;

    [[nodiscard]] auto begin() const noexcept { return map.begin(); }
    [[nodiscard]] auto end()   const noexcept { return map.end(); }

    [[nodiscard]] auto begin() noexcept { return map.begin(); }
    [[nodiscard]] auto end()   noexcept { return map.end(); }

    [[nodiscard]] auto size()  const noexcept { return map.size(); }
    [[nodiscard]] auto empty() const noexcept { return map.empty(); }

private:
    std::unordered_map<std::string, section> map;
};

}

// -------------------------------------------------------- //
//                       code start                         //
// -------------------------------------------------------- //


// entry code ----------------------------------------------//

dot::entry::entry(std::string::const_iterator begin, std::string::const_iterator end)
{
    auto res0 = ParseUtils::parse<double>(begin, end);
    if(res0.has_value()){ var.var = res0.value(); return; }

    auto res1 = ParseUtils::parse<long>(begin, end);
    if(res1.has_value()){ var.var = res1.value(); return; }

    auto res2 = ParseUtils::parse<bool>(begin, end);
    if(res2.has_value()){ var.var = static_cast<long>(res2.value()); return; }

    auto res3 = ParseUtils::parse<std::string>(begin, end);
    if(res3.has_value()){ var.var = res3.value(); return; }

    auto res4 = ParseUtils::parseVector<double>(begin, end);
    if(res4.has_value()){ var.var = res4.value(); return; }

    auto res5 = ParseUtils::parseVector<long>(begin, end);
    if(res5.has_value()){ var.var = res5.value(); return; }

    throw std::runtime_error("no suitable value could b parsed from: " + std::string(begin, end));
}

const dot::inivalue& dot::entry::value() const
{
    if(std::holds_alternative<std::monostate>(var.var)) throw std::runtime_error("entry does not exist");
    return var;
}

bool dot::entry::has_value() const noexcept
{
    return not std::holds_alternative<std::monostate>(var.var);
}

template<typename... Tp>
dot::inivalue dot::entry::value_or(Tp&&... values) const noexcept
{
    if(has_value()) return value();

    if constexpr(sizeof...(Tp) == 1)
    {
        return std::get<0>(std::forward_as_tuple(values...));
    }
    else
    {
        using T = std::tuple_element_t<0,  typename std::tuple<Tp...>>;
        if      constexpr (std::is_integral_v<T>) return std::vector<long>{std::forward<Tp>(values)...};
        else if constexpr (std::is_floating_point_v<T>) return std::vector<double>{std::forward<Tp>(values)...};
        else static_assert(false_type<T>::value, "vector type not supported");
    }
}

template<typename... Tp>
void dot::entry::write(Tp&&... values)
{
    using T = std::tuple_element_t<0,  typename std::tuple<Tp...>>;

    if constexpr(sizeof...(Tp) == 0) var.var = {};
    else if constexpr(sizeof...(Tp) == 1)
    {
        const auto& first = std::get<0>(std::tuple(values...));

        if      constexpr (std::is_integral_v<T>) var.var = static_cast<long>(first);
        else if constexpr (std::is_floating_point_v<T>) var.var = static_cast<double>(first);
        else if constexpr (std::is_convertible_v<T, std::string>) var.var = std::string(first);
        else static_assert(false_type<T>::value, "type not supported");
    }
    else
    {
        // TODO: do some check for all params instead of one
        if      constexpr(std::is_integral_v<T>) var.var = std::vector<long>{std::forward<Tp>(values)...};
        else if constexpr(std::is_floating_point_v<T>) var.var = std::vector<double>{std::forward<Tp>(values)...};
        else static_assert(false_type<T>::value, "vector type not supported");
    }
}

// section code --------------------------------------------//

template<typename T>
const dot::entry& dot::section::operator[](const T& key) const noexcept
{
    static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
    const auto iter = map.find(key);

    if(iter == map.end()) return newEntry;
    else return iter->second;
}

template<typename T>
dot::entry& dot::section::operator[](const T& key) noexcept
{
    static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
    return map.try_emplace(key).first->second;
}

// ini code -------------------------------------------------//

dot::ini::ini(const char* path)
{
    const std::string data = ParseUtils::readToString(path);
    auto current = data.begin();
    auto section = map.end();
    size_t line = 1;

    // seems scary but enough end of file checks are done, trust me
    while (true)
    {
        // create new section
        if (*current == '[')
        {
            const auto iter = ParseUtils::findCharOnLine(current, data.end(), ']');
            if (not iter.has_value())
                throw std::runtime_error("did not ']' before end of file or line on line: " + std::to_string(line));
            section = map.try_emplace({current + 1, iter.value()}, dot::section{}).first;
            current = iter.value();
        }
            // skip over comment
        else if (*current == '#')
        {
        }
            // throw error for whitespace
        else if (ParseUtils::isWhiteSpace(*current))
        {
            const auto next = ParseUtils::skipWhiteSpace(current, data.end());
            if (next.has_value()) current = next.value();
            else return;

            if (*current == '#') {}
            else throw std::runtime_error("whitespace at start of line: " + std::to_string(line));
        } else if (*current == '\n' or *current == '\r')
        {
            ParseUtils::skipLine(current, data.end());
        }
            // if there is an active section, every line is an entry
        else if (section != map.end())
        {
            const auto iter = ParseUtils::findCharOnLine(current, data.end(), '=');
            if (not iter.has_value()) throw std::runtime_error("could not find = on line: " + std::to_string(line));

            const auto nameIter = ParseUtils::skipWhiteSpaceReverse(iter.value() - 1);
            const auto varIter = ParseUtils::skipWhiteSpace(iter.value() + 1, data.end());
            if (not varIter.has_value()) throw std::runtime_error("no value after = on line: " + std::to_string(line));

            const auto endVarIter = ParseUtils::findCharOnLine(varIter.value(), data.end(), '\n');

            // construct a new entry in the section
            auto result = section->second.map.try_emplace({current, nameIter + 1}, entry{varIter.value(), endVarIter.value_or(data.end())});
            if (not result.second) throw std::runtime_error("duplicate value on line: " + std::to_string(line));
            if (not endVarIter.has_value()) return;
        } else
        {
            throw std::runtime_error("assigning elements without section on line: " + std::to_string(line));
        }

        line++;
        const auto next = ParseUtils::skipLine(current, data.end());
        if (next.has_value()) current = next.value();
        else return;
    }
}

std::ofstream& operator<<(std::ofstream& ofstream, const dot::ini& ini)
{
    ofstream << std::fixed << std::setprecision(1);
    for(const auto& section : ini)
    {
        if(section.second.empty()) continue;
        ofstream << '[' << section.first << "]\n";

        for(const auto& entry : section.second)
        {
            if(not entry.second.has_value()) continue;
            auto var = entry.second.variant();

            ofstream << entry.first << " = ";
            switch(var.index())
            {
                case 0: break;
                case 1: ofstream << std::get<double>(var); break;
                case 2: ofstream << std::get<long>(var); break;
                case 3: ofstream << '"' << std::get<std::string>(var) << '"'; break;
                case 4: ofstream << std::get<std::vector<double>>(var); break;
                case 5: ofstream << std::get<std::vector<long>>(var); break;
            }
            ofstream << '\n';
        }
    }
    return ofstream;
}

template<typename T>
const dot::section& dot::ini::operator[](const T& key) const
{
    static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
    const auto iter = map.find(key);

    if(iter == map.end()) throw std::runtime_error("section with key: " + std::string(key) + " does not exist");
    else return iter->second;
}

template<typename T>
dot::section& dot::ini::operator[](const T& key) noexcept
{
    static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
    return map.try_emplace(key).first->second;
}
