#include "configuration.h"
#include <iostream>


int main()
{
    const Configuration config("test.ini");

    float a               = config["section"]["var" ].value();
    std::string b         = config["section"]["var1"].value();
    bool c                = config["section"]["var2"].value_or(true);
    bool d                = config["section"]["var3"].has_value();
    std::vector<float> e  = config["section"]["var5"].value();

    std::cout << a << ' ' << b << ' ' << (c ? "true" : "false") << ' ' << d << ' ' << e[2] << '\n';

    return 0;
}