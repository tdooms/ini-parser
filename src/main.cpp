#include "ini.h"

static void function(const dot::entry& entry, void* a)
{
    long& temp = *reinterpret_cast<long*>(a);
    temp = entry.value();
    std::cout << "value changed to: " << temp << '\n';
}

int main()
{
    long v = 5;
    dot::ini ini("test.ini");
    ini["Section"]["var1"].attach_callback(function, &v);
    ini["Section"]["var1"].change(8);
    ini["Section"]["var1"].change(12);

    const double& test = ini["Section"]["var0"].value();

    ini["Section"]["var0"].change(8.6);

    std::cout << v << " " << test << '\n';

    return 0;
}