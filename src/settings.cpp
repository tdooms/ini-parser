//============================================================================
// @name        : settings.h
// @author      : Thomas Dooms
// @date        : 8/20/19
// @version     : 0.1
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description :
//============================================================================

#include "settings.h"

[[nodiscard]] std::string dot::iniparser::read_to_string(const std::string& path)
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

[[nodiscard]] constexpr bool dot::iniparser::is_whitespace(char c) noexcept
{
    return c == ' ' or c == '\t' or c == '\r';
}

[[nodiscard]] constexpr bool dot::iniparser::is_number(char c) noexcept
{
    return c >= '0' and c <= '9';
}

[[nodiscard]] constexpr bool dot::iniparser::is_character(char c) noexcept
{
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}

[[nodiscard]] dot::iterator dot::iniparser::skip_whitespace(iterator begin, iterator end) noexcept
{
    for(; begin != end; begin++)
    {
        if(not is_whitespace(*begin)) return begin;
    }
    return end;
}

[[nodiscard]] inline dot::iterator dot::iniparser::find_token_end(iterator begin, iterator end) noexcept
{
    for(; begin != end; begin++)
    {
        if(not (is_character(*begin) or is_number(*begin))) return begin;
    }
    return end;
}

[[nodiscard]] inline dot::iterator dot::iniparser::skip_line(iterator begin, iterator end) noexcept
{
    for(; begin != end-1; begin++)
    {
        if(*begin == '\n') return ++begin;
    }
    return end;
}

[[nodiscard]] inline dot::iterator dot::iniparser::skip_line(iterator begin) noexcept
{
    for(; *begin != '\0'; begin++)
    {
        if(*begin == '\n') return ++begin;
    }
    return begin;
}

[[nodiscard]] inline std::pair<dot::iterator, bool> dot::iniparser::skip_empty_line(iterator begin, iterator end) noexcept
{
    bool is_empty = true;
    for(; begin != end-1; begin++)
    {
        if(*begin == '\n') return {++begin, is_empty};
        if(not is_whitespace(*begin)) is_empty = false;
    }
    return {end, is_empty};
}

[[nodiscard]] inline dot::iterator dot::iniparser::find_string_end(dot::iterator begin, dot::iterator end) noexcept
{
    begin++;
    for(; begin != end; begin++)
    {
        if(*begin == '"' and *(begin-1) != '\\') return ++begin;
    }
    return end;
}

//------------------------------------------------//

dot::settings::settings(std::string file_path) : path(std::move(file_path))
{
    const std::string data = iniparser::read_to_string(path);
    const auto end = data.end();

    auto current = data.begin();
    auto line = 1;

    while(true)
    {
        if(*current == '[')
        {
            const auto next = iniparser::find_token_end(++current, end);
            if( next == end) error("file end before closing ]", line);
            if(*next != ']') error("did not find closing ] after section name", line);

            const auto name = std::string(current, next);
            const auto result = map.emplace_back(name, dot::section());
            const auto iter = std::find_if(map.begin(), map.end(), [name](const auto& elem){ return elem.first == name; });
            if(iter == map.end()) error("duplicate section name: ", current, next, line);

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
            if(map.empty()) error("variable has no section", line);

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
                map.back().second[std::string(current, token_end)] = entry(tuple, is_tuple);
                current = current_var + 1;
            }
            else
            {
                auto&& [next, variable] = parse_variable(current_var, end, line);
                map.back().second[std::string(current, token_end)] = entry(variable);
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

std::pair<bool, dot::inivariable::ini_tuple_element> dot::settings::parse_tuple_element(dot::iterator& begin, dot::iterator end, int line, char close)
{
    begin = dot::iniparser::skip_whitespace(begin+1, end);
    auto&& [next, variable] = parse_variable(begin, end, line);
    begin = dot::iniparser::skip_whitespace(next, end);

    if     ( begin == end  ) error("end of file before tuple end", line);
    else if(*begin == ','  ) return {false, std::forward<dot::inivariable::ini_tuple_element>(variable)};
    else if(*begin == close) return {true , std::forward<dot::inivariable::ini_tuple_element>(variable)};
    else error("could not find next , or closing brace after value", begin+1, line);
    throw std::runtime_error("serious error");
}

std::pair<dot::iterator, dot::inivariable::ini_tuple_element> dot::settings::parse_variable(dot::iterator begin, dot::iterator end, int line) noexcept
{
    constexpr const char* false_str = "false";
    constexpr const char* true_str = "true";

    if(*begin == '"')
    {
        auto result = dot::iniparser::find_string_end(begin, end);
        return {result, std::string(begin+1, result-1)};
    }
    else if(std::equal(begin, begin+5, false_str))
    {
        return {begin+5, false};
    }
    else if(std::equal(begin, begin+4, true_str))
    {
        return {begin+4, true};
    }
    char* temp;

    auto current = begin;
    for(;current != end and dot::iniparser::is_number(*current); current++);

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

dot::settings::~settings()
{
    if(path.empty()) return;
    std::ofstream file(path);
    file << *this;
}

void dot::settings::error(const char* first, dot::iterator begin, int line)
{
    error(first, begin, iniparser::skip_line(begin)-2, line);
}
void dot::settings::error(const char* first, dot::iterator begin, dot::iterator end, int line)
{
    const std::string err = std::string(first) + std::string(": \"") + std::string(begin, end) + "\" on line: " + std::to_string(line);
    throw std::runtime_error(err);
}
void dot::settings::error(const char* first, int line)
{
    const std::string err = first + std::string(" on line: ") + std::to_string(line);
    throw std::runtime_error(err);
}