#include <fstream>
#include "settings.h"

static void function(const dot::entry& entry, [[maybe_unused]] void* a)
{
    std::cout << "value changed to: " << entry << '\n';
}

int main()
{
    dot::settings ini("test.ini");
    ini["Section"]["var1"].attach_callback(function);
    ini["Section"]["var1"].change(8);
    ini["Section"]["var1"].change("string");
    ini["Section"]["var1"].write_or_change(1, "string", 5);

    ini["Section"]["var0"].change(8.6);

    return 0;
}