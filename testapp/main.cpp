#include <iostream>
#include "engine/hello.h"
#include "engine/engine.h"
#include <iostream>

#include <entt/entt.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <ranges>



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
    size_t MAX_FRAME_IN_FLIGHT = 3;
    auto bad_function = [](const size_t& i, const size_t& iterations) {
        return (i - 1) % iterations;
    };


    auto good_function = [](const size_t& i, const size_t& MAX_FRAME_IN_FLIGHT) {
        return (i + MAX_FRAME_IN_FLIGHT - 1) % MAX_FRAME_IN_FLIGHT;
    };


    for (size_t i=1;i<=MAX_FRAME_IN_FLIGHT;++i) {
        std::cout << bad_function(i,MAX_FRAME_IN_FLIGHT)<<" ";
    }
    std::cout << std::endl;
    for (size_t i=0;i<MAX_FRAME_IN_FLIGHT;++i) {
        std::cout << bad_function(i,MAX_FRAME_IN_FLIGHT)<<" ";
    }
    std::cout << std::endl;
     for (size_t i=0;i<MAX_FRAME_IN_FLIGHT;++i) {
        std::cout << good_function(i,MAX_FRAME_IN_FLIGHT)<<" ";
    }
    std::cout << std::endl;
    std::vector<size_t> reference(MAX_FRAME_IN_FLIGHT);
    reference[0] = MAX_FRAME_IN_FLIGHT - 1;
    if (reference.size() > 1) {
        std::iota(reference.begin() + 1, reference.end(), 0);
    }

    //return 0;
    // std::cout << std::endl;
    // for (size_t i=0;i<iterations;++i) {
        
    //     std::cout << reference[i]<<" ";
    // }
    // return 0;
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
