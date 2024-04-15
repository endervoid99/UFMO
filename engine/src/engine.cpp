#include "engine/engine.h"
#include "spdlog/spdlog.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <chrono>
#include <thread>

#include "engine/vk_types.h"
#include "vk_initializers.h"
#include "VkBootstrap.h"

constexpr bool bUseValidationLayers = false;

UFMOEngine *loadedEngine = nullptr;


VkBool32 vkDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT messageType,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                void *pUserData)
{
    auto severity = vkb::to_string_message_severity(messageSeverity);
    auto type = vkb::to_string_message_type(messageType);
    switch (messageSeverity)
    {

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        spdlog::info("{}: {}",type,pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        spdlog::warn("{}: {}",type,pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        spdlog::error("{}: {}",type,pCallbackData->pMessage);
        break;
    default:
    }    
    return VK_FALSE;
}

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

uint8_t UFMOEngine::init_vulkan()
{


    spdlog::info("UFMOEngine::init vulkan");
    vkb::InstanceBuilder builder;

    PFN_vkDebugUtilsMessengerCallbackEXT callback = &vkDebugMessageCallback;
#if defined(_DEBUG)        
    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(true)
                        .set_debug_callback(callback)
                        .require_api_version(1, 3, 0)
                        .set_allocation_callbacks(allocatorCallback)
                        //.enable_layer("VK_LAYER_LUNARG_api_dump")
                        .build();

#else
    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(false)
                        //.use_default_debug_messenger()
                        .set_debug_callback(callback)
                        //.set_debug_callback
                        .require_api_version(1, 3, 0)
                        .set_allocation_callbacks(allocatorCallback)
                        //.enable_layer("VK_LAYER_LUNARG_api_dump")
                        .build();
#endif
    if (!inst_ret)
    {
        spdlog::critical("Failed to create Vulkan instance. Error: {} ", inst_ret.error().message());
        return 1;
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

    // TODO: multi gpu systems
    VkPhysicalDeviceFeatures features10{};
    // features10.samplerAnisotropy = false;
    //  use vkbootstrap to select a gpu.
    //  We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
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

    // use vkbootstrap to get a Graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    return 0;

    // spdlog::info("UFMOEngine::init vulkan finished");
}

uint8_t UFMOEngine::init()
{
#if defined _DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
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

    init_commands();

    init_sync_structures();

    // everything went fine
    _isInitialized = true;
    return 0;
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

        draw();
    }
}
void UFMOEngine::create_swapchain(uint32_t width, uint32_t height)
{
    spdlog::info("UFMOEngine::create swapchain");
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

    swapchain.swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{.format = swapchain.swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) //VK_PRESENT_MODE_FIFO_KHR VK_PRESENT_MODE_IMMEDIATE_KHR VK_PRESENT_MODE_MAILBOX_KHR
                                      //.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                                      .set_desired_extent(width, height)
                                      .set_allocation_callbacks(allocatorCallback)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : can render directly to that image
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT: render to seperate image first (for example for post-processing) and TRANSFER to the swap chain image


    swapchain.swapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    swapchain.swapchain = vkbSwapchain.swapchain;
    swapchain.swapchainImages = vkbSwapchain.get_images().value();
    spdlog::debug("UFMOEngine::create swapchain: {} images created",swapchain.swapchainImages.size());
    swapchain.swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void UFMOEngine::init_swapchain()
{
    spdlog::info("UFMOEngine::init swapchain");
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void UFMOEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(_device, swapchain.swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < swapchain.swapchainImageViews.size(); i++)
    {

        vkDestroyImageView(_device, swapchain.swapchainImageViews[i], nullptr);
    }
}

void UFMOEngine::init_commands()
{
    spdlog::info("UFMOEngine::init commands");
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily;

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {

        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = _frames[i]._commandPool;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void UFMOEngine::init_sync_structures()
{
    spdlog::info("UFMOEngine::init sync structures");
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
    
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo,  VK_NULL_HANDLE, &_frames[i]._renderFence));

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo,  VK_NULL_HANDLE, &_frames[i]._swapchainSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo,  VK_NULL_HANDLE, &_frames[i]._renderSemaphore));
	}
}

void UFMOEngine::draw()
{
    //spdlog::info("...");
	// wait until the gpu has finished rendering the last frame. Timeout of 1
	// second
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    //request image from the swapchain
	uint32_t swapchainImageIndex;
                                                  
    //if ( int i = VK_TIMOUT );

    auto result = vkAcquireNextImageKHR(_device, swapchain.swapchain, 1000000000, get_current_frame()._swapchainSemaphore,  VK_NULL_HANDLE, &swapchainImageIndex);

    	//naming it cmd for shorter writing
	
    auto cmdPool = get_current_frame()._commandPool;

    VK_CHECK(vkResetCommandPool(_device,cmdPool,0));
    auto cmd = get_current_frame()._mainCommandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	//VK_CHECK(vkResetCommandBuffer(cmd, 0));

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//start the command buffer recording
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));


   // if (result == VK_TIMEOUT)

	//VK_CHECK(vkAcquireNextImageKHR(_device, swapchain.swapchain, 1000000000, get_current_frame()._swapchainSemaphore,  VK_NULL_HANDLE, &swapchainImageIndex));
}

