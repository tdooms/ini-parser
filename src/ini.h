//============================================================================
// @name        : ini.h
// @author      : Thomas Dooms
// @date        : 8/20/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once

#include <iostream>
#include <memory>
#include <variant>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace dot
{
    enum class ini_type{error, floating, integer, string};
    using iterator = std::string::const_iterator;

    template<typename T>
    struct false_type : std::false_type {};

    template<typename... Types>
    constexpr bool is_vector_ini_type()
    {
        return std::conjunction_v<std::is_integral<Types>...> or
               std::conjunction_v<std::is_floating_point<Types>...> or
               std::conjunction_v<std::is_convertible<std::string, Types>...>;
    }

    template<typename Type>
    constexpr bool check_exact_type() noexcept
    {
        return std::is_same_v<Type, double> or std::is_same_v<Type, long> or std::is_same_v<Type, std::string>;
    }

    template<typename Type>
    constexpr bool check_convertible_type() noexcept
    {
        return std::is_integral_v<Type> or std::is_floating_point_v<Type> or std::is_convertible_v<Type, std::string>;
    }

    template <typename T, typename T2=void> struct type_converter;
    template <> struct type_converter<const char*> { using type = std::string; };
    template <> struct type_converter<std::string> { using type = std::string; };

    template <> struct type_converter<float > { using type = double; };
    template <> struct type_converter<double> { using type = double; };

    template <typename T>
    struct type_converter<T, typename std::enable_if<std::is_integral_v<T>>::type> { using type = long; };

    struct parsedata
    {
        parsedata() = delete;

        iterator begin;
        iterator end;
        ini_type type;
        bool operator==(const parsedata& other) const noexcept { return type == other.type; }
    };

    struct iniparser
    {
        static std::string read_to_string(const std::string &path)
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

        static bool is_whitespace(char c)
        {
            return c == ' ' or c == '\t' or c == '\r';
        }

        static bool is_number(char c)
        {
            return c >= '0' and c <= '9';
        }

        static bool is_character(char c)
        {
            return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
        }

        static iterator skip_whitespace(iterator begin, iterator end)
        {
            for(; begin != end; begin++)
            {
                if(not is_whitespace(*begin)) return begin;
            }
            return end;
        }

        static iterator find_token_end(iterator begin, iterator end)
        {
            for(; begin != end; begin++)
            {
                if(not (is_character(*begin) or is_number(*begin))) return begin;
            }
            return end;
        }

        static iterator skip_line(iterator begin, iterator end)
        {
            for(; begin != end-1; begin++)
            {
                if(*begin == '\n') return ++begin;
            }
            return end;
        }

        static std::pair<iterator, bool> skip_empty_line(iterator begin, iterator end)
        {
            bool is_empty = true;
            for(; begin != end-1; begin++)
            {
                if(*begin == '\n') return {++begin, is_empty};
                if(not is_whitespace(*begin)) is_empty = false;
            }
            return {end, is_empty};
        }

        static std::vector<parsedata> split_tokens(iterator begin, iterator end)
        {
            std::vector<parsedata> result;
            begin = iniparser::skip_whitespace(begin, end);
            if(*begin == '(' or *begin == '[')
            {
                begin++;
                while(begin < end)
                {
                    result.push_back(check_and_skip_token(begin, end));
                    begin = skip_whitespace(result.back().end, end);
                    if(*begin == ')' or * begin == ']') return result;
                    else if(*begin++ != ',') throw std::runtime_error("no ',' between values");
                }
            }
            else
            {
                result.push_back(check_and_skip_token(begin, end));
                return result;
            }
            throw std::runtime_error("internal error");
        }

        static parsedata check_and_skip_token(iterator begin, iterator end)
        {
            begin = iniparser::skip_whitespace(begin, end);
            if(*begin == '"')
            {
                for(auto current = begin; current != end; current++)
                {
                    if(*current == '"' and current != begin and *(current-1) != '\\') return {begin, ++current, ini_type::string};
                }
                return {begin, end, ini_type::error};
            }
            auto current = begin;
            if(*current == '+' or *current == '-') current++;
            if(not is_number(*current) or *current == '.') return {begin, end, ini_type::error};
            for(; is_number(*current); current++);

            if(*current == '.') current++;
            else return { begin, current, ini_type::integer };

            for(; is_number(*current); current++);
            return {begin, current, ini_type::floating};
        }

        static long parse_long(iterator begin, iterator end) noexcept
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

        static double parse_double(iterator begin, iterator end) noexcept
        {
            const bool negative = (*begin == '-');
            double result = 0;
            double pow = 0.1;

            if(*begin == '+' or *begin == '-') begin++;

            while(begin != end and *begin >= '0' and *begin <= '9')
            {
                result *= 10;
                result += *begin - '0';
                begin++;
            }

            begin++; // *begin == '.'

            while(begin != end and *begin >= '0' and *begin <= '9')
            {
                result += (*begin - '0') * pow;
                pow *= 0.1;
                begin++;
            }

            return negative ? -result : result;
        }
    };

    class inivariable
    {
        using ini_tuple_element = std::variant<double, long, std::string>;
        using ini_element = std::variant<std::monostate, double, long, std::string, std::vector<double>, std::vector<long>,  std::vector<std::string>, std::vector<ini_tuple_element>>;

    public:
        explicit inivariable() = default;

        template<typename... Types>
        explicit inivariable(Types&&... types)
        {
            using Type = std::tuple_element_t<0, typename std::tuple<Types...>>;

            if constexpr (sizeof...(Types) == 1)
            {
                auto first = std::get<0>(std::tuple(types...));

                if constexpr(check_convertible_type<Type>()) entry = static_cast<typename type_converter<Type>::type>(first);
                else if constexpr (std::is_same_v<Type, std::vector<parsedata>>) fill_entry(first, entry);
                else static_assert(false_type<Type>::value, "type for variable not supported");
            }
            else if constexpr(is_vector_ini_type<Types...>())
            {
                if constexpr(check_convertible_type<Type>()) entry = std::vector<typename type_converter<Type>::type >{std::forward<Types>(types)...};
                else static_assert(false_type<Type>::value, "type for vector not supported");
            }
            else
            {
                entry = std::vector<ini_tuple_element>(sizeof...(Types));
                fill_vector(std::get<std::vector<ini_tuple_element>>(entry), std::forward<Types>(types)...);
            }
        }

        [[nodiscard]] constexpr inline auto index() const noexcept { return entry.index(); }

        template<typename T>
        [[nodiscard]] operator const T&() const noexcept
        {
            static_assert(check_exact_type<T>(), "please only use values that consist of: double, long, std::string");
            return std::get<T>(entry);
        }

        template<typename T>
        [[nodiscard]] operator T() const noexcept
        {
            static_assert(check_exact_type<T>(), "please only use values that consist of: double, long, std::string");
            return std::get<T>(entry);
        }

        template<typename T>
        [[nodiscard]] operator const std::vector<T>&() const noexcept
        {
            static_assert(check_exact_type<T>(), "please only use vectors that consist of: double, long, std::string");
            return std::get<std::vector<T>>(entry);
        }

        template<typename T>
        [[nodiscard]] operator std::vector<T>() const noexcept
        {
            static_assert(check_exact_type<T>(), "please only use vectors that consist of: double, long, std::string");
            return std::get<std::vector<T>>(entry);
        }

        template<typename... Ts>
        [[nodiscard]] operator std::tuple<Ts...>() const noexcept
        {
            std::tuple<Ts...> tuple;
            fill_tuple(std::get<std::vector<ini_tuple_element>>(entry), tuple);
            return tuple;
        }

    private:
        static void fill_entry(const std::vector<parsedata>& vec, ini_element& var)
        {
            if(vec.size() == 1)
            {
                if     (vec.front().type == ini_type::integer ) var.emplace<long>(iniparser::parse_long  (vec.front().begin, vec.front().end));
                else if(vec.front().type == ini_type::floating) var.emplace<double>(iniparser::parse_double(vec.front().begin, vec.front().end));
                else if(vec.front().type == ini_type::string  ) var.emplace<std::string>(vec.front().begin+1, vec.front().end-1);
            }
            else if(std::equal(vec.begin()+1, vec.end(), vec.begin()))
            {
                if(vec.front().type == ini_type::integer)
                {
                    var.emplace<std::vector<long>>();
                    for(const auto& elem : vec) std::get<std::vector<long>>(var).push_back(iniparser::parse_long(elem.begin, elem.end));
                }
                else if(vec.front().type == ini_type::floating)
                {
                    var.emplace<std::vector<double>>();
                    for(const auto& elem : vec) std::get<std::vector<double>>(var).push_back(iniparser::parse_double(elem.begin, elem.end));
                }
                else if(vec.front().type == ini_type::string)
                {
                    var.emplace<std::vector<std::string>>();
                    for(const auto& elem : vec) std::get<std::vector<std::string>>(var).emplace_back(elem.begin, elem.end);
                }
                else throw std::runtime_error("weird internal error");
            }
            else
            {
                var.emplace<std::vector<ini_tuple_element>>(vec.size());
                auto& tuple = std::get<std::vector<ini_tuple_element>>(var);
                for(size_t i = 0; i < vec.size(); i++)
                {
                    auto& tuple_elem = tuple[i];
                    if     (vec[i].type == ini_type::integer ) tuple_elem = iniparser::parse_long  (vec[i].begin, vec[i].end);
                    else if(vec[i].type == ini_type::floating) tuple_elem = iniparser::parse_double(vec[i].begin, vec[i].end);
                    else if(vec[i].type == ini_type::string  ) tuple_elem = std::string(vec[i].begin+1, vec[i].end-1);
                }
            }
        }

        template<size_t I = 0, typename... Types>
        static void fill_vector(std::vector<ini_tuple_element>& vec, Types&&... types)
        {
            using Type = std::tuple_element_t<I, typename std::tuple<Types...>>;
            auto&& elem = std::get<I>(std::tuple(types...));

            if constexpr(check_convertible_type<Type>()) vec[I] = static_cast<typename type_converter<Type>::type>(elem);
            else static_assert(false_type<Type>::value, "type for tuple not supported");

            if constexpr (I+1 == sizeof...(Types)) return;
            else fill_vector<I+1, Types...>(vec, std::forward<Types>(types)...);
        }

        template<size_t I = 0, typename... Types>
        static void fill_tuple(const std::vector<ini_tuple_element>& vec, std::tuple<Types...>& tuple)
        {
            using Type = std::tuple_element_t<I, typename std::tuple<Types...>>;

            if constexpr(check_convertible_type<Type>()) std::get<I>(tuple) = static_cast<typename type_converter<Type>::type>(vec[I]);
            else static_assert(false_type<Type>::value, "type for tuple not supported");

            if constexpr (I+1 == sizeof...(Types)) return;
            else fill_tuple<I+1, Types...>(vec, tuple);
        }

        ini_element entry;
    };


    class entry
    {
    public:
        explicit entry() = default;

        template<typename... Types>
        explicit entry(Types&&... types) : variable(std::forward<Types>(types)...) {}


        [[nodiscard]] bool empty() const noexcept
        {
            return variable.index() == 0;
        }
        [[nodiscard]] bool is_variable() const noexcept
        {
            return variable.index() > 0 and variable.index() < 4;
        }

        [[nodiscard]] bool is_vector() const noexcept
        {
            return variable.index() > 3 and variable.index() < 7;
        }

        [[nodiscard]] bool is_tuple() const noexcept
        {
            return variable.index() == 7;
        }

        void attach_callback(std::function<void(const entry&, void*)> fn, void* args) const noexcept
        {
            callback = std::move(fn);
            callback_data = args;
        }

        [[nodiscard]] const inivariable& value() const
        {
            if(empty()) throw std::runtime_error("accessing empty variable");
            return variable;
        }

        template<typename... Types>
        [[nodiscard]] inivariable value_or(Types&&... types) const
        {
            if(empty()) return IniVariable(std::forward<Types>(types)...);
            return variable;
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return not empty();
        }

        template<typename... Types>
        void write(Types&&... types)
        {
            if(not empty()) throw std::runtime_error("existing variable cannot be written to, try 'change' or 'write_or_change' ");
            else write_or_change(std::forward<Types>(types)...);
        }

        template<typename... Types>
        void change(Types&&... types)
        {
            if(empty()) throw std::runtime_error("empty variable cannot be changed, try 'write' or 'write_or_change' ");
            else write_or_change(std::forward<Types>(types)...);
        }

        template<typename... Types>
        void write_or_change(Types&&... types) noexcept
        {
            variable = inivariable(std::forward<Types>(types)...);
            if(callback != nullptr) callback(*this, callback_data);
        }

    private:
        inivariable variable;

        mutable std::function<void(const entry&, void*)> callback = nullptr;
        mutable void* callback_data = nullptr;
    };

    class section
    {
        friend class ini;

    public:
        section() = default;

        template<typename T>
        entry& operator[](const T& key) noexcept
        {
            static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to std::string");
            return map.try_emplace(key).first->second;
        }

        template<typename T>
        const entry& operator[](const T& key) const noexcept
        {
            static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to std::string");
            auto iter = map.find(key);

            if(iter != map.end()) return iter->second;
            else return item;
        }

    private:
        std::unordered_map<std::string, entry> map;
        inline static const entry item = entry();
    };

    class ini
    {
    public:
        ini() = default;

        explicit ini(const std::string& path)
        {
            const std::string data = iniparser::read_to_string(path);
            const auto end = data.end();

            auto current = data.begin();
            auto section = map.end();
            auto line = 1;

            while(true)
            {
                if(*current == '[')
                {
                    const auto next = iniparser::find_token_end(++current, end);
                    if( next == end) error("file end before closing ]", line);
                    if(*next != ']') error("did not find closing ] after section name", line);

                    const auto result = map.try_emplace(std::string(current, next), dot::section());
                    if(not result.second) error("duplicate section name: ", current, next, line);
                    section = result.first;

                    const auto line_end = iniparser::skip_empty_line(next+1, end);
                    if(not line_end.second) error("symbols found after section name", next+1, line_end.first-1, line);
                    else current = line_end.first;
                }
                else if(*current == '#' or *current == ';')
                {
                    current = iniparser::skip_line(current, end);
                }
                else if(iniparser::is_whitespace(*current))
                {
                    auto result = iniparser::skip_empty_line(current, end);
                    if(not result.second) error("please do not use whitespace before data", line);
                    else current = result.first;
                }
                else if(iniparser::is_character(*current))
                {
                    if(section == map.end()) error("variable has no section", line);

                    const auto token_end = iniparser::find_token_end(current, end);
                    if(token_end == end) error("end of file before variable name end", line);

                    auto eq_end = iniparser::skip_whitespace(token_end, end);
                    if(eq_end == end) error("end of file after variable name", line);

                    if(*eq_end++ != '=') error("could not find '='", line);

                    auto&& vec = iniparser::split_tokens(eq_end, end);
                    auto next = iniparser::skip_line(eq_end, end);

                    for(const auto& elem : vec)
                    {
                        if(elem.type == ini_type::error) error("could not parse value", eq_end, next-2, line);
                    }

                    section->second.map.try_emplace(std::string(current, token_end), entry(std::forward<std::vector<parsedata>>(vec)));
                    current = next;
                }

                if(current == end) return;
                line++;
            }
        };

        template<typename T>
        section& operator[](const T& key) noexcept
        {
            static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to std::string");
            return map.try_emplace(key).first->second;
        }

        template<typename T>
        const section& operator[](const T& key) const
        {
            static_assert(std::is_convertible_v<T, std::string>, "key type not convertible to std::string");
            auto iter = map.find(key);

            if(iter != map.end()) return iter->second;
            else throw std::runtime_error("could not find section with key" + std::string(key));
        }

    private:
        static void error(const char* first, iterator begin, iterator end, int line)
        {
            const std::string err = std::string(first) + std::string(": \"") + std::string(begin, end) + "\" on line: " + std::to_string(line);
            throw std::runtime_error(err);
        }
        static void error(const char* first, int line)
        {
            const std::string err = first + std::string(" on line: ") + std::to_string(line);
            throw std::runtime_error(err);
        }

        std::unordered_map<std::string, section> map;
    };


}