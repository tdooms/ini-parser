# ini settings manager

This is a settings manager based on ini files for storage.
The goal of this manager is to easily store/change data and react to those changes.

## Features

Every ini entry consists of the types: (long, double, string).
every entry can be one of these types, a vector of these types or a tuple of these types.

Every time you query a value, it wil detect which type it must return, 
this way you don't have to cast every time you want an unsigned int instead of int.

```bash 
    double a                = config["Section"]["var0"].value();
    std::string b           = config["Section"]["var1"].value();
    std::vector<float> c    = config["Section"]["var5"].value();
    bool d                  = config["Section"]["var2"].has_value();
    int e                   = config["Section"]["var3"].value_or(3);
    std::vector<int> f      = config["Section"]["var4"].value_or(1,2,3,4);
    bool g                  = config["Section"]["var6"].empty();
    bool h                  = config["Section"]["var6"].is_tuple();
    std::tuple<float, int>  = config["Section"]["var6"].value_or(5.2, 4);
```

We can change data and attach a function when a value changes.
We also attach a void* to pass data when the function is called.

```bash 
    static void function(const dot::entry& entry, void* data)
    {
        const long& temp = entry.value();
        std::cout << "value changed to: " << temp << '\n';
    }

    ini["Section"]["var1"].attach_callback(function);
    ini["Section"]["var1"].write(8);
    ini["Section"]["var1"].change(12);

    >> value changed to: 8
    >> value changed to: 12
```


It is also possible to load/write/store very easily

```bash 
    dot::ini config("test.ini");
    config["section"]["var0"].write(1, "string");

    std::ofstream file("test2.ini");
    file << config;
```

## future

Better support for iterators
