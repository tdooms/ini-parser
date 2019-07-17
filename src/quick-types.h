//============================================================================
// @name        : quick-types.h
// @author      : Thomas Dooms
// @date        : 6/24/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================


#pragma once

#include <immintrin.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

// This is a string that has at most 16 characters,
// we use this to our advantage so that we can compare the whole string at once,
// which makes this very suitable for fast map keys.
namespace dot
{
    struct short_string
    {
        short_string(const char* string)
        {
            data = _mm_set1_epi32(0);
            auto begin = reinterpret_cast<char*>(&data);
            for (auto i = 0; i < 16; i++)
            {
                if (string[i] == '\0') break;
                else begin[i] = string[i];
            }
        }

        short_string(std::string::const_iterator begin, std::string::const_iterator end)
        {
            data = _mm_set1_epi32(0);
            memcpy(&data, begin.base(), static_cast<size_t>(end - begin));
        }

        short_string(const std::string& string)
        {
            data = _mm_set1_epi32(0);
            memcpy(&data, begin(string).base(), string.size());
        }

        inline bool operator==(const short_string& rhs) const noexcept
        {
            auto result = _mm_movemask_epi8(_mm_cmpeq_epi8(data, rhs.data));
            return result == 0x0000ffff;
        }

        __m128i data;
    };

    // a vector map is better than a real map for small sizes.
    template<typename Type>
    struct quick_map
    {
        using iterator = typename std::vector<std::pair<short_string, Type>>::iterator;
        using const_iterator = typename std::vector<std::pair<short_string, Type>>::const_iterator;

        quick_map(){ data.reserve(8); }

        template<typename T>
        [[nodiscard]] const_iterator find(const T& key) const noexcept
        {
            static_assert(std::is_convertible<T, short_string>(), "type not convertible to internal string");
            const short_string temp = key;
            return std::find_if(begin(), end(), [temp](const auto& elem){ return elem.first == temp; });
        }

        std::pair<iterator, bool> try_emplace(short_string&& key, Type&& value)
        {
            if(find(key) != end()) return { end(), false};
            data.emplace_back(std::forward<short_string>(key), std::forward<Type>(value));
            return { --end(), true };
        }

        const_iterator end() const noexcept { return data.end(); }
        iterator end() noexcept { return data.end(); }

        const_iterator begin() const noexcept { return data.begin(); }
        iterator begin() noexcept { return data.begin(); }

        std::vector<std::pair<short_string, Type>> data;
    };
}
