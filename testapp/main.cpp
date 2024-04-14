#include <iostream>
#include "engine/hello.h"
#include "engine/engine.h"

int main(int, char**){    
    auto engine = new UFMOEngine();
    engine->init();
    //engine->init_vulkan();
    engine->run();
    engine->cleanup();
    //std::string helloJim = generateHelloString("Jim");
    //std::cout << helloJim << std::endl;
    //std::cout << "Hello, from test222!\n";
}
