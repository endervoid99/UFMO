#include "engine/hello.h"

#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <Eigen/Dense>
#include "spdlog/spdlog.h"

#include "VkBootstrap.h"

#define SPDLOG_ACTIVE_LEVEl SPDLOG_LEVEL_ERROR

using Eigen::MatrixXd;

using namespace std;

const string generateHelloString(const string &personName)
{
    vkb::InstanceBuilder builder;
auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(false)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

    //     vkb::InstanceBuilder builder;
    // auto inst_ret = builder.set_app_name ("Example Vulkan Application")
    //                     .request_validation_layers ()
    //                     .use_default_debug_messenger ()
    //                     .build ();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
        return false;
    }

        vkb::Instance vkb_inst = inst_ret.value ();

// /*     vkb::PhysicalDeviceSelector selector{ vkb_inst };
// /*     auto phys_ret = selector.set_surface (/* from user created window*/)
//                         .set_minimum_version (1, 1) // require a vulkan 1.1 capable device
//                         .require_dedicated_transfer_queue ()
//                         .select ();
//     if (!phys_ret) {
//         std::cerr << "Failed to select Vulkan Physical Device. Error: " << phys_ret.error().message() << "\n";
//         return false;
//     }

//     vkb::DeviceBuilder device_builder{ phys_ret.value () };
//     // automatically propagate needed data from instance & physical device
//     auto dev_ret = device_builder.build ();
//     if (!dev_ret) {
//         std::cerr << "Failed to create Vulkan device. Error: " << dev_ret.error().message() << "\n";
//         return false;
//     }
//     vkb::Device vkb_device = dev_ret.value ();

//     // Get the VkDevice handle used in the rest of a vulkan application
//     VkDevice device = vkb_device.device;

//     // Get the graphics queue with a helper function
//     auto graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
//     if (!graphics_queue_ret) {
//         std::cerr << "Failed to get graphics queue. Error: " << graphics_queue_ret.error().message() << "\n";
//         return false;
//     }
//     VkQueue graphics_queue = graphics_queue_ret.value (); */

//     // Turned 400-500 lines of boilerplate into less than fifty. */
   

    MatrixXd m(2, 2);
    m(0, 0) = 3;
    m(1, 0) = 2.5;
    m(0, 1) = -1;
    m(1, 1) = m(1, 0) + m(0, 1);
    std::cout << m << std::endl;
    glm::mat4 mat;
    spdlog::info("Welcome to spdlog!");
    spdlog::error("Some error message with arg: {}", 1);
    SPDLOG_INFO("TEST");
        
    return "Hello2 " + personName;
    
}