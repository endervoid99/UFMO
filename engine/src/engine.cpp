#include "engine/engine.h"
#include "spdlog/spdlog.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <chrono>
#include <thread>

#include "engine/vk_types.h"
#include "VkBootstrap.h"

constexpr bool bUseValidationLayers = false;

UFMOEngine *loadedEngine = nullptr;

UFMOEngine &UFMOEngine::get() { return *loadedEngine; }

void UFMOEngine::cleanup()
{
    spdlog::info("UFMOEngine::cleanup");
    if (_isInitialized)
    {
		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);
		
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
		SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void UFMOEngine::init_vulkan()
{
    spdlog::info("UFMOEngine::init vulkan");
    vkb::InstanceBuilder builder;

    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(true)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        //.enable_layer("VK_LAYER_LUNARG_api_dump")                        
                        .build();
     if (!inst_ret) {
        spdlog::info("Failed to create Vulkan instance. Error: "+ inst_ret.error().message());
        //std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
        return;
    }
    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    _instance = vkb_inst.instance;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    

    VkPhysicalDeviceFeatures features10{};
    //features10.samplerAnisotropy = false;
    // use vkbootstrap to select a gpu.
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDevice = selector
                                             .set_minimum_version(1, 3)
                                             .set_required_features_13(features)
                                             .set_required_features_12(features12)
                                             //.set_required_features(features10)
                                             .set_surface(_surface)
                                             .select()
                                             .value();

    // create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    _debug_messenger = vkb_inst.debug_messenger;
    // spdlog::info("UFMOEngine::init vulkan finished");
}

void UFMOEngine::init()
{
    spdlog::info("UFMOEngine::init");
    // only one engine initialization is allowed with the application.
    // assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);


    
	init_vulkan();

	init_swapchain();

	//init_commands();

	//init_sync_structures();

    // everything went fine
    _isInitialized = true;
    // spdlog::info("UFMOEngine::init finished");
}

void UFMOEngine::run()
{
    spdlog::info("UFMOEngine::run");
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit)
    {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT)
            {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED)
                {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering)
        {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // draw();
    }
}
void UFMOEngine::create_swapchain(uint32_t width, uint32_t height)
{   
    spdlog::info("UFMOEngine::create swapchain");
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(width, height)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    _swapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
}


void UFMOEngine::init_swapchain()
{
    spdlog::info("UFMOEngine::init swapchain");
	create_swapchain(_windowExtent.width, _windowExtent.height);
}

void UFMOEngine::destroy_swapchain()
{
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// destroy swapchain resources
	for (int i = 0; i < _swapchainImageViews.size(); i++) {

		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}
