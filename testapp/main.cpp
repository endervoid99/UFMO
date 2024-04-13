#include <iostream>
#include "engine/hello.h"

int main(int, char**){    
    std::string helloJim = generateHelloString("Jim");
    std::cout << helloJim << std::endl;
    std::cout << "Hello, from test222!\n";
}
