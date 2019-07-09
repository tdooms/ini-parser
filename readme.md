# Quick ini parser

This is an ini parser focused on speed and the ability to extend it yourself. This parser also has exceptional exception handling.

## Speed

At the moment every key name can be at most 16 characters long, this improves speed by 10x. I also don't use maps but vectors for improved cache locality stuff.

I will continue to explore further improvements and remove the 16 character limit while keeping performance.

## Extensions

You can add your own types to be parsed. This is done like this:
* add your own type to the list or make your own
```bash 
using Section = VarMap<std::string, double, int, bool, TYPE>;
```
* write your own parser for the type and put it in cparse.h, this is the function skeleton
```bash 
template<>
std::optional<TYPE> parse(std::string::const_iterator begin, std::string::const_iterator end) noexcept {}
```

Note: When a parser returns a complete value, that value will be used. Define them with care!

## Behind the ADT wall
An ini file exists of sections with entries in them. Every entry is a std::variant of user-specified types. The core parser will give the user freedom to define these types and call their parse functions.

The biggest problem with allowing the user to give as many types as they want is calling the right parsers in sequence without any overhead. This is done with some recursive template hackery.

```bash
template<uint32_t I = 0>  
static Variable read(std::string::const_iterator begin, std::string::const_iterator end)  
{  
  if constexpr (I == sizeof...(Types)) throw std::runtime_error();
  auto result = parse<typename std::tuple_element<I, typename std::tuple<Types...>>::type>(begin, end);  
  if(result.has_value()) return result.value();  
  return read<I+1>(begin, end);  
}
  ```

We also need to make implicit conversions for every possible type, so that we can use the [][] operator.

```bash
template<typename T>  
operator T() const  
{  
  static_assert(std::disjunction<std::is_same<T, Types>...>(), "type not supported");  
  return std::get<T>(var);  
}
  ```

The static assert gives the user a compile time error when they request the wrong type.

All of this gives us a beautiful and simple result.

```bash
std::string b = config["section"]["var1"].value();  
bool c = config["section"]["var2"].value_or(true);  
bool d = config["section"]["var3"].has_value();
  ```

## Parsing

I used C libraries to copy the whole text into a buffer to read, we then iterate over it without taking any copies!

## future
I plan to further optimize/extend this parser and enable std::cout on entries.

There is still an error when GCC compiler optimizations are turned on which a plan to fix as soon as i know what it is.

```bash
warning: ‘<anonymous>’ may be used uninitialized in this function [-Wmaybe-uninitialized]
   42 |     return {};
```

