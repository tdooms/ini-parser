//============================================================================
// @name        : settings.h
// @author      : Thomas Dooms
// @date        : 8/20/19
// @version     : 0.1
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

template<typename T>
constexpr size_t type_index()
{
    if      constexpr(std::is_integral_v<T>) return 0;
    else if constexpr(std::is_floating_point_v<T>) return 1;
    else if constexpr(std::is_convertible_v<T, std::string>) return 2;
    else return 3;
}
template<size_t I> struct type_converter;
template<> struct type_converter<0> { using type = long; };
template<> struct type_converter<1> { using type = double; };
template<> struct type_converter<2> { using type = std::string; };
template<> struct type_converter<3> { using type = void; };

template<typename T>
using type_converter_t = typename type_converter<type_index<T>()>::type;


struct iniparser
{
    static std::string read_to_string(const std::string& path)
    {
        auto file = fopen(path.c_str(), "rb");
        if(not file) throw std::runtime_error("could not open file: " + path);
        fseek(file, 0, SEEK_END);
        auto size = static_cast<size_t>(ftell(file));
        fseek(file, 0, SEEK_SET);

        std::string string(size, 0);
        fread(string.data(), 1, size, file);
        fclose(file);
        return string;
    }

    [[nodiscard]] constexpr inline static bool is_whitespace(char c) noexcept
    {
        return c == ' ' or c == '\t' or c == '\r';
    }

    [[nodiscard]] constexpr inline static bool is_number(char c) noexcept
    {
        return c >= '0' and c <= '9';
    }

    [[nodiscard]] constexpr inline static bool is_character(char c) noexcept
    {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
    }

    [[nodiscard]] inline static iterator skip_whitespace(iterator begin, iterator end) noexcept
    {
        for(; begin != end; begin++)
        {
            if(not is_whitespace(*begin)) return begin;
        }
        return end;
    }

    [[nodiscard]] inline static iterator find_token_end(iterator begin, iterator end) noexcept
    {
        for(; begin != end; begin++)
        {
            if(not (is_character(*begin) or is_number(*begin))) return begin;
        }
        return end;
    }

    [[nodiscard]] inline static iterator skip_line(iterator begin, iterator end) noexcept
    {
        for(; begin != end-1; begin++)
        {
            if(*begin == '\n') return ++begin;
        }
        return end;
    }

    [[nodiscard]] inline static iterator skip_line(iterator begin) noexcept
    {
        for(; *begin != '\0'; begin++)
        {
            if(*begin == '\n') return ++begin;
        }
        return begin;
    }

    [[nodiscard]] inline static std::pair<iterator, bool> skip_empty_line(iterator begin, iterator end) noexcept
    {
        bool is_empty = true;
        for(; begin != end-1; begin++)
        {
            if(*begin == '\n') return {++begin, is_empty};
            if(not is_whitespace(*begin)) is_empty = false;
        }
        return {end, is_empty};
    }

    [[nodiscard]] inline static iterator find_string_end(iterator begin, iterator end) noexcept
    {
        begin++;
        for(; begin != end; begin++)
        {
            if(*begin == '"' and *(begin-1) != '\\') return ++begin;
        }
        return end;
    }
};

class inivariable
{
public:
    using ini_tuple_element = std::variant<double, long, std::string>;
    using ini_element = std::variant<std::monostate, double, long, std::string, std::vector<double>, std::vector<long>,  std::vector<std::string>, std::vector<ini_tuple_element>>;

    explicit inivariable() = default;

    template<typename... Types>
    explicit inivariable(Types&&... types)
    {
        using Type = std::tuple_element_t<0, typename std::tuple<Types...>>;

        if constexpr (sizeof...(Types) == 1)
        {
            auto first = std::get<0>(std::tuple(types...));
            if constexpr      (check_convertible_type<Type>()) entry = static_cast<type_converter_t<Type>>(first);
            else if constexpr (std::is_same_v<Type, ini_tuple_element>) find_index(std::forward<ini_tuple_element>(first), entry);
            else static_assert(false_type<Type>::value, "type for variable not supported");
        }
        else if constexpr (sizeof...(Types) == 2 and std::is_same_v<Type, std::vector<ini_tuple_element>>)
        {
            auto first = std::get<0>(std::tuple(types...));
            if(std::get<1>(std::tuple(types...))) entry.emplace<std::vector<ini_tuple_element>>(first);
            else check_and_convert_vector(std::forward<std::vector<ini_tuple_element>>(first), entry);
        }
        else if constexpr (is_vector_ini_type<Types...>()) //clang nonsense
        {
            if constexpr(check_convertible_type<Type>()) entry = std::vector<type_converter_t<Type>>{std::forward<Types>(types)...};
            else static_assert(false_type<Type>::value, "type for vector not supported");
        }
        else
        {
            entry = std::vector<ini_tuple_element>(sizeof...(Types));
            fill_vector(std::get<std::vector<ini_tuple_element>>(entry), std::forward<Types>(types)...);
        }
    }

    [[nodiscard]] constexpr inline auto index() const noexcept { return entry.index(); }

    template<typename T, std::enable_if_t<check_exact_type<T>(), int> = 0>
    [[nodiscard]] operator const T&() const noexcept
    {
        return std::get<T>(entry);
    }

    template<typename T>
    [[nodiscard]] operator T() const noexcept
    {
        static_assert(check_convertible_type<T>(), "please only use values that can convert to: double, long, std::string");
        return static_cast<T>(std::get<type_converter_t<T>>(entry));
    }

    template<typename T>
    [[nodiscard]] operator const std::vector<T>&() const noexcept
    {
        static_assert(check_exact_type<T>() or std::is_same_v<T, ini_tuple_element>, "please only use vectors that consist of: double, long, std::string");
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
    static void check_and_convert_vector(std::vector<ini_tuple_element>&& vec, ini_element& entry)
    {
        auto index = vec.front().index();
        if(not std::all_of(vec.begin(), vec.end(), [index](const auto& iter){ return iter.index() == index; }))
            throw std::runtime_error("not all elements in vector are same type, please use (..) for a tuple");

        if     (index == 0) fill_vector<double     >(std::forward<std::vector<ini_tuple_element>>(vec), entry);
        else if(index == 1) fill_vector<long       >(std::forward<std::vector<ini_tuple_element>>(vec), entry);
        else if(index == 2) fill_vector<std::string>(std::forward<std::vector<ini_tuple_element>>(vec), entry);
    }

    template<typename T>
    static void fill_vector(std::vector<ini_tuple_element>&& vec, ini_element& entry)
    {
        auto type_vec = entry.emplace<std::vector<T>>(vec.size());
        for(size_t i = 0; i < vec.size(); i++) type_vec[i] = std::get<T>(vec[i]);
    }

    template<size_t I = 0>
    static void find_index(ini_tuple_element&& elem, ini_element& entry) noexcept
    {
        if constexpr(std::variant_size_v<ini_tuple_element> == I) return;
        else
        {
            if(elem.index() == I) entry = std::get<std::variant_alternative_t<I, ini_tuple_element>>(elem);
            else find_index<I+1>(std::forward<ini_tuple_element>(elem), entry);
        }
    }

    template<size_t I = 0, typename... Types>
    static void fill_vector(std::vector<ini_tuple_element>& vec, Types&&... types) noexcept
    {
        using Type = std::tuple_element_t<I, typename std::tuple<Types...>>;
        auto&& elem = std::get<I>(std::tuple(types...));

        if constexpr(check_convertible_type<Type>()) vec[I] = static_cast<type_converter_t<Type>>(elem);
        else static_assert(false_type<Type>::value, "type for tuple not supported");

        if constexpr (I+1 == sizeof...(Types)) return;
        else fill_vector<I+1, Types...>(vec, std::forward<Types>(types)...);
    }

    template<size_t I = 0, typename... Types>
    static void fill_tuple(const std::vector<ini_tuple_element>& vec, std::tuple<Types...>& tuple) noexcept
    {
        using Type = std::tuple_element_t<I, typename std::tuple<Types...>>;

        if constexpr(check_convertible_type<Type>()) std::get<I>(tuple) = std::get<type_converter_t<Type>>(vec[I]);
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
    [[nodiscard]] bool has_value() const noexcept
    {
        return not empty();
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

    void attach_callback(std::function<void(const entry&, void*)> fn, void* args = nullptr) const noexcept
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

    void erase() noexcept
    {
        variable = inivariable();
    }

    template<typename T>
    friend T& operator<<(T& stream, const entry& entry)
    {
        switch(entry.variable.index())
        {
            case 0: break;
            case 1: stream << static_cast<const double&     >(entry.variable); break;
            case 2: stream << static_cast<const long&       >(entry.variable); break;
            case 3: stream << '"' << static_cast<const std::string&>(entry.variable) << '"'; break;
            case 4: print(stream, static_cast<const std::vector<double           >&>(entry.variable)); break;
            case 5: print(stream, static_cast<const std::vector<long             >&>(entry.variable)); break;
            case 6: print(stream, static_cast<const std::vector<std::string      >&>(entry.variable)); break;
            case 7: print(stream, static_cast<const std::vector<inivariable::ini_tuple_element>&>(entry.variable)); break;
            default: break;
        }
        return stream;
    }

private:
    template<typename T>
    static T& print(T& stream, const std::vector<inivariable::ini_tuple_element>& vec) noexcept
    {
        stream << '(';
        for(size_t i = 0; i < vec.size(); i++)
        {
            switch(vec[i].index())
            {
                case 0: stream << std::get<double     >(vec[i]); break;
                case 1: stream << std::get<long       >(vec[i]); break;
                case 2: stream << '"' <<std::get<std::string>(vec[i]) << '"'; break;
            }
            stream << ((i == vec.size()-1) ? ")" : ", ");
        }
        return stream;
    }

    template<typename T, typename V>
    static T& print(T& stream, const std::vector<V>& vec) noexcept
    {
        stream << '[';
        if constexpr(std::is_same_v<V, std::string>) for(size_t i = 0; i < vec.size()-1; i++) stream << '"' << vec[i] << "\", ";
        else for(size_t i = 0; i < vec.size()-1; i++) stream << vec[i] << ", ";
        return stream << vec.back() << ']';
    }

    inivariable variable;

    mutable std::function<void(const entry&, void*)> callback = nullptr;
    mutable void* callback_data = nullptr;
};

class section
{
    friend class settings;

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

    [[nodiscard]] auto begin() const noexcept { return map.begin(); }
    [[nodiscard]] auto end() const noexcept { return map.end(); }

    [[nodiscard]] auto begin() noexcept { return map.begin(); }
    [[nodiscard]] auto end() noexcept { return map.end(); }

    [[nodiscard]] auto empty() noexcept { return map.empty(); }
    [[nodiscard]] auto size() noexcept { return map.size(); }

private:
    std::unordered_map<std::string, entry> map;
    inline static const entry item = entry();
};

class settings
{
public:
    settings() = default;

    explicit settings(std::string file_path) : path(std::move(file_path))
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

                const auto var_begin = iniparser::skip_whitespace(token_end, end);
                if(*var_begin != '=') error("could not find '='", line);
                auto current_var = iniparser::skip_whitespace(var_begin+1, end);


                if(*current_var == '(' or *current_var == '[')
                {
                    std::vector<inivariable::ini_tuple_element> tuple;
                    bool done = false;
                    bool is_tuple = *current_var == '(';

                    while(not done)
                    {
                        auto&& [new_done, var] = parse_tuple_element(current_var, end, line, (is_tuple) ? ')' : ']');
                        tuple.emplace_back(std::forward<inivariable::ini_tuple_element>(var));
                        done = new_done;
                    }
                    section->second.map.try_emplace(std::string(current, token_end), std::forward<std::vector<inivariable::ini_tuple_element>>(tuple), is_tuple);
                    current = current_var + 1;
                }
                else
                {
                    auto&& [next, variable] = parse_variable(current_var, end, line);
                    section->second.map.try_emplace(std::string(current, token_end), std::forward<inivariable::ini_tuple_element>(variable));
                    current = next;
                }
                auto&& [end_line, is_empty] = iniparser::skip_empty_line(current, end);
                if(not is_empty) error("line not empty after variable", line);
                current = end_line;
            }

            if(current == end) return;
            line++;
        }
    }

    ~settings()
    {
        std::ofstream file(path);
        file << *this;
    }

    [[nodiscard]] auto begin() const noexcept { return map.begin(); }
    [[nodiscard]] auto end() const noexcept { return map.end(); }

    [[nodiscard]] auto begin() noexcept { return map.begin(); }
    [[nodiscard]] auto end() noexcept { return map.end(); }

    [[nodiscard]] auto empty() noexcept { return map.empty(); }
    [[nodiscard]] auto size() noexcept { return map.size(); }

    template<typename T>
    friend T& operator<<(T& stream, const dot::settings& ini)
    {
        for(const auto& section_data : ini)
        {
            const auto& section = section_data.second;
            if( std::none_of(section.begin(), section.end(), [](const auto& data){ return data.second.has_value(); }) ) continue;

            stream << '[' << section_data.first << "]\n";
            for(const auto& entry_data : section)
            {
                stream << entry_data.first << " = " << entry_data.second << '\n';
            }
        }
        return stream;
    }

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
    static std::pair<bool, dot::inivariable::ini_tuple_element> parse_tuple_element(iterator& begin, iterator end, int line, char close)
    {
        begin = iniparser::skip_whitespace(begin+1, end);
        auto&& [next, variable] = parse_variable(begin, end, line);
        begin = iniparser::skip_whitespace(next, end);

        if     ( begin == end  ) error("end of file before tuple end", line);
        else if(*begin == ','  ) return {false, std::forward<inivariable::ini_tuple_element>(variable)};
        else if(*begin == close) return {true , std::forward<inivariable::ini_tuple_element>(variable)};
        else error("could not find next , or closing brace after value", begin+1, line);
        throw std::runtime_error("serious error");
    }

    static std::pair<iterator, dot::inivariable::ini_tuple_element> parse_variable(iterator begin, iterator end, int line) noexcept
    {
        constexpr const char* false_str = "false";
        constexpr const char* true_str = "true";

        if(*begin == '"')
        {
            auto result = iniparser::find_string_end(begin, end);
            return {result, std::string(begin+1, result-1)};
        }
        else if(std::equal(begin, begin+5, false_str))
        {
            return {begin+5, 0l};
        }
        else if(std::equal(begin, begin+4, true_str))
        {
            return {begin+4, 1l};
        }
        char* temp;

        auto current = begin;
        for(;current != end and iniparser::is_number(*current); current++);

        if(*current == '.')
        {
            double res = std::strtod(begin.base(), &temp);
            if(temp == begin.base()) error("could not parse value", line);
            return { static_cast<iterator>(temp), res };
        }
        else
        {
            long res = std::strtol(begin.base(), &temp, 10);
            if(temp == begin.base()) error("could not parse value", line);
            return { static_cast<iterator>(temp), res };
        }
    }

    static void error(const char* first, iterator begin, int line)
    {
        error(first, begin, iniparser::skip_line(begin)-2, line);
    }
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

    std::string path;
    std::unordered_map<std::string, section> map;
};


}