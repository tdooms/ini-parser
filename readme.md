# settings settings manager

This is a settings manager based on settings files for storage.
The goal of this manager is to easily store/change data and react to those changes.

## Features

Every settings entry consists of the types: (long, double, string).
every entry can be one of these types, a vector of these types or a tuple of these types.

Every time you query a value, it wil detect which type it must return, 
this way you don't have to cast every time you want an unsigned int instead of int.

Here are some examples of how easily you can use the settings manager.

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

If there is no value stored, you have to write, if there is one, you have to change.
If you don't want to think about it, use write_or_change.

```bash 
    static void function(const dot::entry& entry, [[maybe_unused]] void* a)
    {
        std::cout << "value changed to: " << entry << '\n';
    }

    settings["Section"]["var1"].attach_callback(function);
    settings["Section"]["var1"].write(8);
    settings["Section"]["var1"].change("string");
    settings["Section"]["var1"].write_or_change(1, "string", 5);

    >> value changed to: 8
    >> value changed to: "string"
    >> value changed to: (1, "string", 5)
```

As the aim of the settings class is to be stored, it writes its changes back to the file when deleted.
Making a program to write to the settings would be as easy as this.

```bash 
    dot::settings config("test.settings");
    config["section"]["var0"].write(1, "string");

```
