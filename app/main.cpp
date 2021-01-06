
#include <iostream>
#include <stdio.h>
#include <string>

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        std::cout << "forgot argument :P" << std::endl;
        return -1;
    }

    std::cout << "yo!" << std::endl;

    return 0;
}
