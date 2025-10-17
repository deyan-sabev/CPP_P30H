#include <iostream>
#include <exception>

#include "program.hpp"

int main(int argc, char** argv)
{
    try
    {
        return program::run(argc, argv);
    }
    catch(const std::exception& e)
    {
        std::cerr << '\n' << e.what() << '\n' << std::endl;
    }
}
