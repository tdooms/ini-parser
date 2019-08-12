# Easy ini parser

This is an ini parser focused on ease of use.
The goalwas to make the syntax as simple and expressive as possible.

## Features

Every ini entry can be (long, double, string, vector<long>, vector<double>).
Every time you query a value, it wil detect which type it must return, 
this way you don't have to cast everytime you want an unsigned int instead of int.

```bash 
    double a                = config["section"]["var"].value();
    std::string b           = config["section"]["var1"].value();
    std::vector<float> c    = config["section"]["var5"].value();
    bool d                  = config["section"]["var2"].has_value();
    int e                   = config["section"]["var3"].value_or(3);
    std::vector<int> f      = config["section"]["var4"].value_or(1,2,3,4);
```

You can also load/write/store very easily

```bash 
    dot::ini config("test.ini");
    config["section"]["var0"].write(1., 2.);

    std::ofstream file("test2.ini");
    file << config;
```

## Behind the ADT wall

By using type traits we can go very far to make sure almost every type is supported upon querying,
and everything is done at compile time.

You can easily add your own code here to support some strange types.
Otherwise you can support type trait on your class and make them castable.

```bash 
template<typename T>
[[nodiscard]] operator T() const noexcept
{
    if      constexpr (std::is_integral_v      <T>) return static_cast<T>(std::get<long>(var));
    else if constexpr (std::is_floating_point_v<T>) return static_cast<T>(std::get<double>(var));
    else if constexpr (std::is_convertible_v<T, std::string>) return std::get<std::string>(var);
    else static_assert(false_type<T>::value, "type not supported");
}
```
