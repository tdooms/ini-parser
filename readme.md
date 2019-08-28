# settings manager

This is a settings manager based on ini files for storage.\
The goal of this manager is to easily store/change data and react to those changes.

## How to use

Every entry can consist of (bool, double, long, string), 
a vector of these types or a tuple of these types.

```bash 
[Section]
var0 = 8.84
var1 = false
var2 = "string"
var3 = [1,2,3,4]
var4 = (true, "string", 5)
```

Every time you query a value, it will automatically detect which type it must return.

```bash 
float a                              = settings["Section"]["var0"].value();
bool b                               = settings["Section"]["var1"].value();
std::string c                        = settings["Section"]["var2"].value();
std::vector<uint32_t> d              = settings["Section"]["var3"].value();
std::tuple<bool, std::string, int> e = settings["Section"]["var4"].value();
```

Here are some helper functions for getting the value.

```bash 
float a = settings["Section"]["var0"].value_or(5.0); // 8.84
float b = settings["Section"]["var5"].value_or(5.0); // 5.0
bool c  = settings["Section"]["var5"].has_value();   // false

std::tuple<bool, std::string, int> d = settings["Section"]["var4"].value_or(true, "hey", 42);
```

We can write/change and erase data.\
Writing only works when the variable does not exist, if it does exist, you can change it.\
If you don't want to think about it, you can write_or_change.

```bash 
settings["Section"]["var5"].write(1,2,3);
settings["Section"]["var0"].change("string");

settings["Section"]["var1"].write(5);           // error
settings["Section"]["var1"].write_or_change(5); // no error
```

We can attach a function when a value changes.\
We can also attach a void* to pass data when the function is called.\

```bash 
static void function(const dot::entry& entry, [[maybe_unused]] void* a)
{
    std::cout << "value changed to: " << entry << '\n';
}

settings["Section"]["var1"].attach_callback(function);
settings["Section"]["var1"].write(8);
settings["Section"]["var1"].change("string");
settings["Section"]["var1"].write_or_change(1, "string", 5);
```

```bash 
>> value changed to: 8
>> value changed to: "string"
>> value changed to: (1, "string", 5)
```

As the aim of the settings class is to be stored, it writes its changes back to the file when deleted.
Making a program to write to the settings would be as easy as this.

```bash 
dot::settings settings("test.ini");
settings["section"]["var0"].write(1, "string");
```

There is support for iterating over the settings/sections

```bash 
dot::settings settings("test.ini");
for(const auto& elem : settings["Section"])
{   
    std::cout << "name : " << elem.first  << '\n';
    std::cout << "value: " << elem.second << '\n';
}
```