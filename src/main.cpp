#include "settings.h"


int main()
{
    dot::settings settings("test.ini");
    std::cout << settings << '\n';
    settings["Section"]["var3"].erase();
    settings["Section"]["var0"].change(8.);

    const double& elem = settings["Section"]["var0"].value();
    std::cout << elem << '\n';
//    std::cout << settings["Section"]["var0"] << '\n';
    return 0;
}