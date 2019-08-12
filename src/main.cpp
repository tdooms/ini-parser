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


struct A
{
    template<typename T>
    T add(T type);
    int a = 5;
};

template<typename T>
T A::add(T type){ return type + a; }


int main()
{
//    dot::ini config;
//    config["section"]["entry1"].write(5);
//    config["section"]["entry2"].write("something");
//    config["section"]["entry3"].write(1,2,3,4,5);
//
//    int a                   = config["section"]["entry1"].value();
//    std::string b           = config["section"]["entry2"].value();
//    std::vector<int> c      = config["section"]["entry3"].value();
//    bool d                  = config["section"]["entry4"].value_or(true);
//    std::string e           = config["section"]["entry5"].value_or("string");
//    std::vector<uint32_t> f = config["section"]["entry3"].value_or(2,3,5,7);

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

    std::cout << a << '\n' << b << '\n' << c[2] << '\n' << d << '\n' << e << '\n' << f[3] << '\n';
}