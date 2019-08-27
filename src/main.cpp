#include "settings.h"


int main()
{
    dot::settings settings("test.ini");
    std::cout << settings << '\n';

    return 0;
}