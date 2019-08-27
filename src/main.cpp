#include <fstream>
#include "settings.h"

static void function(const dot::entry& entry, [[maybe_unused]] void* a)
{
    std::cout << "value changed to: " << entry << '\n';
}

int main()
{
    dot::settings ini("test.ini");
    std::cout << ini["Section"]["var4"] << '\n';

    return 0;
}