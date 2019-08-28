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
#include <algorithm>
#include <fstream>
#include <functional>

namespace dot
{
    using iterator = std::string::const_iterator;

    template<typename T>
    struct false_type : std::false_type {};

    template<typename... Types>
    constexpr inline bool is_vector_ini_type_v = std::conjunction_v<std::is_integral<Types>...> or
                                                 std::conjunction_v<std::is_floating_point<Types>...> or
                                                 std::conjunction_v<std::is_convertible<std::string, Types>...>;

    template<typename Type>
    constexpr inline bool is_stored_type_v = std::is_same_v<Type, bool> or std::is_same_v<Type, double> or std::is_same_v<Type, long> or std::is_same_v<Type, std::string>;

    template<typename Type>
    constexpr inline bool is_stored_const_ref_v = is_stored_type_v<std::decay<Type>> and std::is_const_v<Type> and std::is_reference_v<Type>;

    template<typename Type>
    constexpr inline bool is_convertible_type_v = std::is_integral_v<Type> or std::is_floating_point_v<Type> or std::is_convertible_v<Type, std::string>;

    template<typename T>
    constexpr size_t type_index()
    {
        if      constexpr(std::is_same_v<T, bool>) return 0;
        else if constexpr(std::is_integral_v<T>) return 1;
        else if constexpr(std::is_floating_point_v<T>) return 2;
        else if constexpr(std::is_convertible_v<T, std::string>) return 3;
        else return 4;
    }

    template<size_t I> struct type_converter;
    template<> struct type_converter<0> { using type = bool; };
    template<> struct type_converter<1> { using type = long; };
    template<> struct type_converter<2> { using type = double; };
    template<> struct type_converter<3> { using type = std::string; };
    template<> struct type_converter<4> { using type = void; };

    template<typename T>
    using type_converter_t = typename type_converter<type_index<T>()>::type;


    struct iniparser
    {
        [[nodiscard]] static std::string read_to_string(const std::string& path);

        [[nodiscard]] constexpr static bool is_whitespace(char c) noexcept;

        [[nodiscard]] constexpr static bool is_number(char c) noexcept;

        [[nodiscard]] constexpr static bool is_character(char c) noexcept;

        [[nodiscard]] static iterator skip_whitespace(iterator begin, iterator end) noexcept;

        [[nodiscard]] static iterator find_token_end(iterator begin, iterator end) noexcept;

        [[nodiscard]] static iterator skip_line(iterator begin, iterator end) noexcept;

        [[nodiscard]] static iterator skip_line(iterator begin) noexcept;

        [[nodiscard]] static std::pair<iterator, bool> skip_empty_line(iterator begin, iterator end) noexcept;

        [[nodiscard]] static iterator find_string_end(iterator begin, iterator end) noexcept;
    };

    class inivariable
    {
    public:
        using ini_tuple_element = std::variant<bool, double, long, std::string>;
        using ini_element = std::variant<std::monostate, bool, double, long, std::string, std::vector<bool>, std::vector<double>, std::vector<long>,  std::vector<std::string>, std::vector<ini_tuple_element>>;

        explicit inivariable() = default;

        template<typename... Types>
        explicit inivariable(Types&&... types)
        {
            using Type = std::decay_t<std::tuple_element_t<0, typename std::tuple<Types...>>>;

            if constexpr (sizeof...(Types) == 1)
            {
                auto first = std::get<0>(std::tuple(types...));
                if constexpr      (is_convertible_type_v<Type>) entry = static_cast<type_converter_t<Type>>(first);
                else if constexpr (std::is_same_v<Type, ini_tuple_element>) find_index(std::forward<ini_tuple_element>(first), entry);
                else static_assert(false_type<Type>::value, "type for variable not supported");
            }
            else if constexpr (sizeof...(Types) == 2 and std::is_same_v<Type, std::vector<ini_tuple_element>>)
            {
                auto first = std::get<0>(std::tuple(types...));
                if(std::get<1>(std::tuple(types...))) entry.emplace<std::vector<ini_tuple_element>>(first);
                else check_and_convert_vector(std::forward<std::vector<ini_tuple_element>>(first), entry);
            }
            else if constexpr (is_vector_ini_type_v<Types...>) //clang nonsense
            {
                if constexpr(is_convertible_type_v<Type>) entry = std::vector<type_converter_t<Type>>{std::forward<Types>(types)...};
                else static_assert(false_type<Type>::value, "type for vector not supported");
            }
            else
            {
                entry = std::vector<ini_tuple_element>(sizeof...(Types));
                fill_vector(std::get<std::vector<ini_tuple_element>>(entry), std::forward<Types>(types)...);
            }
        }

        [[nodiscard]] constexpr size_t index() const noexcept { return entry.index(); }

        template<typename T, std::enable_if_t<is_stored_const_ref_v<T>, int> = 0>
        [[nodiscard]] operator const T&() const noexcept
        {
            return std::get<T>(entry);
        }

        template<typename T>
        [[nodiscard]] operator T() const noexcept
        {
            static_assert(is_convertible_type_v<T>, "please only use types convertible to: double, long or std::string");
            return static_cast<T>(std::get<type_converter_t<T>>(entry));
        }

        template<typename T, std::enable_if_t<is_stored_type_v<T> or std::is_same_v<T, ini_tuple_element>, int> = 0>
        [[nodiscard]] operator const std::vector<T>&() const noexcept
        {
            return std::get<std::vector<T>>(entry);
        }

        template<typename T>
        [[nodiscard]] operator std::vector<T>() const noexcept
        {
            static_assert(is_convertible_type_v<T>, "please only use types convertible to: double, long or std::string");
            const auto& vec = std::get<std::vector<type_converter_t<T>>>(entry);
            return std::vector<T>(vec.begin(), vec.end());
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

            if     (index == 0) fill_vector<bool       >(std::forward<std::vector<ini_tuple_element>>(vec), entry);
            else if(index == 1) fill_vector<double     >(std::forward<std::vector<ini_tuple_element>>(vec), entry);
            else if(index == 2) fill_vector<long       >(std::forward<std::vector<ini_tuple_element>>(vec), entry);
            else if(index == 3) fill_vector<std::string>(std::forward<std::vector<ini_tuple_element>>(vec), entry);
        }

        template<typename T>
        static void fill_vector(std::vector<ini_tuple_element>&& vec, ini_element& entry)
        {
            auto& type_vec = entry.emplace<std::vector<T>>(vec.size());
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
            using Type = std::decay_t<std::tuple_element_t<I, typename std::tuple<Types...>>>;
            auto&& elem = std::get<I>(std::tuple(types...));

            if constexpr(is_convertible_type_v<Type>()) vec[I] = static_cast<type_converter_t<Type>>(elem);
            else static_assert(false_type<Type>::value, "type for tuple not supported");

            if constexpr (I+1 == sizeof...(Types)) return;
            else fill_vector<I+1, Types...>(vec, std::forward<Types>(types)...);
        }

        template<size_t I = 0, typename... Types>
        static void fill_tuple(const std::vector<ini_tuple_element>& vec, std::tuple<Types...>& tuple) noexcept
        {
            using Type = std::decay_t<std::tuple_element_t<I, typename std::tuple<Types...>>>;

            if constexpr(is_convertible_type_v<Type>()) std::get<I>(tuple) = std::get<type_converter_t<Type>>(vec[I]);
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

        [[nodiscard]] constexpr size_t index() const noexcept
        {
            return variable.index();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return index() == 0;
        }
        [[nodiscard]] bool has_value() const noexcept
        {
            return not empty();
        }
        [[nodiscard]] bool is_variable() const noexcept
        {
            return index() > 0 and index() < 5;
        }
        [[nodiscard]] bool is_vector() const noexcept
        {
            return index() > 4 and index() < 9;
        }
        [[nodiscard]] bool is_tuple() const noexcept
        {
            return index() == 9;
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

    private:
        inivariable variable;

        mutable std::function<void(const entry&, void*)> callback = nullptr;
        mutable void* callback_data = nullptr;
    };

    class section
    {
    public:
        section() = default;

        entry& operator[](const std::string& key) noexcept
        {
            const auto iter = std::find_if(map.begin(), map.end(), [&key](const auto& elem){ return elem.first == key; });
            if(iter == map.end()) return map.emplace_back(key, entry()).second;
            else return iter->second;
        }

        const entry& operator[](const std::string& key) const noexcept
        {
            const auto iter = std::find_if(map.begin(), map.end(), [&key](const auto& elem){ return elem.first == key; });
            if(iter == map.end()) return item;
            else return iter->second;
        }

        [[nodiscard]] auto begin() const noexcept { return map.begin(); }
        [[nodiscard]] auto end() const noexcept { return map.end(); }

        [[nodiscard]] auto begin() noexcept { return map.begin(); }
        [[nodiscard]] auto end() noexcept { return map.end(); }

        [[nodiscard]] auto empty() const noexcept { return map.empty(); }
        [[nodiscard]] auto size() const noexcept { return map.size(); }

    private:
        std::vector<std::pair<std::string, entry>> map;
        inline static const entry item = entry();
    };

    struct iniprinter
    {
        template<typename T, typename V>
        static T& print(T& stream, const V& val)
        {
            if      constexpr (std::is_same_v<V, bool>) stream << (val?"true":"false");
            else if constexpr (std::is_same_v<V, std::string>) stream << '"' << val << '"';
            else if constexpr (std::is_same_v<V, dot::inivariable::ini_tuple_element>)
            {
                if     (val.index() == 0) print(stream, std::get<bool       >(val));
                else if(val.index() == 1) print(stream, std::get<double     >(val));
                else if(val.index() == 2) print(stream, std::get<long       >(val));
                else if(val.index() == 3) print(stream, std::get<std::string>(val));
                else throw std::runtime_error("internal error 1");
            }
            else stream << val;
            return stream;
        }

        template<typename T, typename V>
        static T& print(T& stream, const std::vector<V>& vec)
        {
            constexpr auto close = std::is_same_v<V, dot::inivariable::ini_tuple_element> ? std::pair<char, char>{'(', ')'} : std::pair<char, char>{'[', ']'};
            stream << close.first;
            for(size_t i = 0; i < vec.size()-1; i++) print(stream, vec[i]) << ", ";
            print(stream, vec.back());
            stream << close.second;
            return stream;
        }

        template<typename T>
        static T& print(T& stream, const entry& entry)
        {
            switch(entry.index())
            {
                case 0: return stream;
                case 1: return print(stream, static_cast<const bool&                                       >(entry.value()));
                case 2: return print(stream, static_cast<const double&                                     >(entry.value()));
                case 3: return print(stream, static_cast<const long&                                       >(entry.value()));
                case 4: return print(stream, static_cast<const std::string&                                >(entry.value()));
                case 5: return print(stream, static_cast<const std::vector<bool>&                          >(entry.value()));
                case 6: return print(stream, static_cast<const std::vector<double>&                        >(entry.value()));
                case 7: return print(stream, static_cast<const std::vector<long>&                          >(entry.value()));
                case 8: return print(stream, static_cast<const std::vector<std::string>&                   >(entry.value()));
                case 9: return print(stream, static_cast<const std::vector<inivariable::ini_tuple_element>&>(entry.value()));
            }
            throw std::runtime_error("internal error 0");
        }

//        template<typename T, size_t I = 0>
//        static T& print(T& stream, const entry& entry)
//        {
//            if constexpr (I == 0 or I == std::variant_size_v<inivariable::ini_element>) return stream;
//            else
//            {
//                if(I == entry.index()) return print(stream, static_cast<const std::variant_alternative_t<I, inivariable::ini_element>&>(entry.value()));
//                else return print<T, I+1>(stream, entry);
//            }
//        }

        template<typename T>
        static T& print(T& stream, const std::string& name, const section& section)
        {
            if( std::none_of(section.begin(), section.end(), [](const auto& data){ return data.second.has_value(); }) ) return stream;
            stream << '[' << name << "]\n";

            for(const auto& elem : section)
            {
                if(elem.second.empty()) continue;
                stream << elem.first << " = ";
                print(stream, elem.second) << '\n';
            }
            stream << '\n';
            return stream;
        }
    };

    class settings
    {
    public:
        settings() = default;

        explicit settings(std::string file_path);

        ~settings();

        [[nodiscard]] auto begin() const noexcept { return map.begin(); }
        [[nodiscard]] auto end() const noexcept { return map.end(); }

        [[nodiscard]] auto begin() noexcept { return map.begin(); }
        [[nodiscard]] auto end() noexcept { return map.end(); }

        [[nodiscard]] auto empty() const noexcept { return map.empty(); }
        [[nodiscard]] auto size() const noexcept { return map.size(); }

        template<typename T>
        friend T& operator<<(T& stream, const settings& settings)
        {
            for(const auto& section_data : settings)
            {
                const auto& [name, section] = section_data;
                iniprinter::print(stream, name, section);
            }
            return stream;
        }

        section& operator[](const std::string& key) noexcept
        {
            const auto iter = std::find_if(map.begin(), map.end(), [&key](const auto& elem){ return elem.first == key; });
            if(iter == map.end()) return map.emplace_back(key, section()).second;
            else return iter->second;
        }

        const section& operator[](const std::string& key) const
        {
            const auto iter = std::find_if(map.begin(), map.end(), [&key](const auto& elem){ return elem.first == key; });
            if(iter != map.end()) return iter->second;
            else throw std::runtime_error("could not find section with key" + std::string(key));
        }

    private:
        static std::pair<bool, dot::inivariable::ini_tuple_element> parse_tuple_element(iterator& begin, iterator end, int line, char close);

        static std::pair<iterator, dot::inivariable::ini_tuple_element> parse_variable(iterator begin, iterator end, int line) noexcept;

        static void error(const char* first, iterator begin, int line);
        static void error(const char* first, iterator begin, iterator end, int line);
        static void error(const char* first, int line);

        std::string path;
        std::vector<std::pair<std::string, section>> map;
    };


}