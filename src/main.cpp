//============================================================================
// @name        : main.cpp
// @author      : Thomas Dooms
// @date        : 8/10/19
// @version     : 
// @copyright   : BA1 Informatica - Thomas Dooms - University of Antwerp
// @description : 
//============================================================================

#include "ini.h"
#include <iostream>


int main()
{
    dot::ini config("test.ini");
    config["section"]["var0"].write(1., 2.);

    double a                = config["section"]["var"].value();
    std::string b           = config["section"]["var1"].value();
    std::vector<float> c    = config["section"]["var5"].value();
    bool d                  = config["section"]["var2"].has_value();
    int e                   = config["section"]["var3"].value_or(3);
    std::vector<int> f      = config["section"]["var4"].value_or(1,2,3,4);

    std::ofstream file("test2.ini");
    file << config;

    for(const auto& section : config)
    {
        std::cout << "section: " << section.first << '\n';
        for(const auto& entry : section.second)
        {
            if(entry.second.has_value()) std::cout << "\tentry: " << entry.first << '\n';
        }
        std::cout << '\n';
    }

    std::cout << a << '\n' << b << '\n' << c[2] << '\n' << d << '\n' << e << '\n' << f[3] << '\n';
}