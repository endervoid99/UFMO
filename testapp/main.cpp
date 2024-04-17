#include <iostream>
#include "engine/hello.h"
#include "engine/engine.h"
#include <iostream>

#include <entt/entt.hpp>

void set(std::span<int32_t> data)
{
    for (int i = 0;auto& e : data)
    {
        e =  i;     
        i++;   
    };
}

void print(std::span<int32_t> data) {
    for (auto &e : data)
    {
        std::cout << e << " ";
    };
}

int main(int, char **)
{
    //int32_t data[32];
    // set(data);
    // print(data);
    // return 0;

    auto engine = new VulkanRenderer();
    engine->init();
    // engine->init_vulkan();
    engine->run();
    engine->tearDown();
    // std::string helloJim = generateHelloString("Jim");
    // std::cout << helloJim << std::endl;
    // std::cout << "Hello, from test222!\n";
}
