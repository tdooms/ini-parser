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
    bool isWhiteSpace(char c);

    const std::string readToString(const std::string &path);

    std::optional<std::string::const_iterator> findCharOnLine(std::string::const_iterator begin, std::string::const_iterator end, char c) noexcept;

    std::optional<std::string::const_iterator> skipLine(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    std::optional<std::string::const_iterator> skipWhiteSpace(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    std::string::const_iterator skipWhiteSpaceReverse(std::string::const_iterator begin) noexcept;


    template<typename T>
    std::optional<T> parse(std::string::const_iterator, std::string::const_iterator) noexcept;

    template<>
    std::optional<long> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    template<>
    std::optional<double> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    template<>
    std::optional<std::string> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    template<>
    std::optional<bool> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept;

    template<typename T>
    std::optional<std::vector<T>> parseVector(std::string::const_iterator begin, std::string::const_iterator end) noexcept;
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
    [[nodiscard]] inivalue value_or(Tp&&... values) const noexcept
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
    void write(Tp&&... values)
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

private:
    inivalue var;
};

class section
{
public:
    friend class ini;

    template<typename T>
    [[nodiscard]] const entry& operator[](const T& key) const noexcept
    {
        static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
        const auto iter = map.find(key);

        if(iter == map.end()) return newEntry;
        else return iter->second;
    }

    template<typename T>
    [[nodiscard]] entry& operator[](const T& key) noexcept
    {
        static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
        return map.try_emplace(key).first->second;
    }

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

    explicit ini(const std::string& path);


    friend std::ofstream& ::operator<<(std::ofstream& ofstream, const dot::ini& ini);

    template<typename T>
    [[nodiscard]] const section& operator[](const T& key) const
    {
        static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
        const auto iter = map.find(key);

        if(iter == map.end()) throw std::runtime_error("section with key: " + std::string(key) + " does not exist");
        else return iter->second;
    }

    template<typename T>
    [[nodiscard]] section& operator[](const T& key) noexcept
    {
        static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to string");
        return map.try_emplace(key).first->second;
    }

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