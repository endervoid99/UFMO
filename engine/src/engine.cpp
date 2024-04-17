#include "engine/engine.h"
#include "spdlog/spdlog.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <chrono>
#include <thread>

#include "engine/vk_types.h"
#include "vk_initializers.h"
#include "VkBootstrap.h"

#include <tracy/Tracy.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

constexpr bool bUseValidationLayers = false;

VulkanRenderer *loadedEngine = nullptr;
VkAllocationCallbacks *AllocatorCallback::p_allocatorCallback = nullptr;

VkBool32 vkDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT messageType,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                void *pUserData)
{
    ZoneScoped;
    auto severity = vkb::to_string_message_severity(messageSeverity);
    auto type = vkb::to_string_message_type(messageType);
    switch (messageSeverity)
    {

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        spdlog::info("{}: {}", type, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        spdlog::warn("{}: {}", type, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        spdlog::error("{}: {}", type, pCallbackData->pMessage);
        cpptrace::generate_trace().print();
        break;
    default:
    }
    return VK_FALSE;
}

Swapchain::Swapchain(BasicVulkanData &vulkanData) : m_vulkanData(vulkanData){

                                                    };

Swapchain::~Swapchain(){

};

void Swapchain::initSwapchain()
{
    ZoneScoped;
    spdlog::info("UFMOEngine::init swapchain");
    createSwapchain(m_vulkanData.windowExtent.width, m_vulkanData.windowExtent.height);
}

void Swapchain::createSwapchain(uint32_t width, uint32_t height)
{
    //--------------------------
    // Swapchain
    //.........................
    ZoneScoped;
    spdlog::info("UFMOEngine::create swapchain");
    vkb::SwapchainBuilder swapchainBuilder{m_vulkanData.chosenGPU, m_vulkanData.device, m_vulkanData.surface};

    m_data.swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      //.use_default_format_selection()
                                      .set_desired_format(VkSurfaceFormatKHR{.format = m_data.swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      // use vsync present mode
                                      //.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) //VK_PRESENT_MODE_FIFO_KHR VK_PRESENT_MODE_IMMEDIATE_KHR VK_PRESENT_MODE_MAILBOX_KHR
                                      /// set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(width, height)
                                      .set_allocation_callbacks(AllocatorCallback::p_allocatorCallback)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : can render directly to that image
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT: render to seperate image first (for example for post-processing) and TRANSFER to the swap chain image

    m_data.swapchainExtent = vkbSwapchain.extent;
    // store swapchain and its related images
    m_data.swapchain = vkbSwapchain.swapchain;
    m_data.swapchainImages = vkbSwapchain.get_images().value();
    spdlog::debug("UFMOEngine::create swapchain: {} images created", m_data.swapchainImages.size());
    m_data.swapchainImageViews = vkbSwapchain.get_image_views().value();

    //------------------------------
    // Images
    //------------------------------
    // draw image size will match the window
    VkExtent3D drawImageExtent = {
        m_vulkanData.windowExtent.width,
        m_vulkanData.windowExtent.height,
        1};

    // hardcoding the draw format to 32 bit float
    m_vulkanData.drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_vulkanData.drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(m_vulkanData.drawImage.imageFormat, drawImageUsages, drawImageExtent);

    // for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(m_vulkanData.allocator, &rimg_info, &rimg_allocinfo, &m_vulkanData.drawImage.image, &m_vulkanData.drawImage.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(m_vulkanData.drawImage.imageFormat, m_vulkanData.drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(m_vulkanData.device, &rview_info, nullptr, &m_vulkanData.drawImage.imageView));

    // add to deletion queues
    m_vulkanData.mainDeletionQueue.push_function([=]()
                                                 {
		vkDestroyImageView(m_vulkanData.device, m_vulkanData.drawImage.imageView, nullptr);
		vmaDestroyImage(m_vulkanData.allocator, m_vulkanData.drawImage.image, m_vulkanData.drawImage.allocation); });
}

VulkanRenderer &VulkanRenderer::get() { return *loadedEngine; }

void VulkanRenderer::tearDown()
{
    ZoneScoped;
    spdlog::info("UFMOEngine::cleanup");
    if (_isInitialized)
    {
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDeviceWaitIdle(vulkanData.device);
            vulkanData.mainDeletionQueue.flush();
            // VK_CHECK(vkWaitForFences(vulkanData.device, 1, &_frames[i]._renderFence, true, 1000000000));
            // already written from before
            vkDestroyCommandPool(vulkanData.device, _frames[i]._commandPool, nullptr);

            // destroy sync objects
            vkDestroyFence(vulkanData.device, _frames[i]._renderFence, nullptr);
            vkDestroySemaphore(vulkanData.device, _frames[i]._renderSemaphore, nullptr);
            vkDestroySemaphore(vulkanData.device, _frames[i]._swapchainSemaphore, nullptr);
        }

        destroySwapchain();

        vkDestroySurfaceKHR(vulkanData.instance, vulkanData.surface, nullptr);
        vkDestroyDevice(vulkanData.device, nullptr);

        vkb::destroy_debug_utils_messenger(vulkanData.instance, vulkanData.debug_messenger);
        vkDestroyInstance(vulkanData.instance, nullptr);
        SDL_DestroyWindow(vulkanData.window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

uint8_t VulkanRenderer::initVulkan()
{
    ZoneScoped;
    spdlog::info("UFMOEngine::init vulkan");
    vkb::InstanceBuilder builder;

    PFN_vkDebugUtilsMessengerCallbackEXT callback = &vkDebugMessageCallback;
#if defined(_DEBUG)
    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(true)
                        .set_debug_callback(callback)
                        .require_api_version(1, 3, 0)
                        .set_allocation_callbacks(AllocatorCallback::p_allocatorCallback)
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
        cpptrace::generate_trace().print();
        return 1;
    }
    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    vulkanData.instance = vkb_inst.instance;

    SDL_Vulkan_CreateSurface(vulkanData.window, vulkanData.instance, &vulkanData.surface);

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
                                             .set_surface(vulkanData.surface)
                                             .select()
                                             .value();

    // create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    vulkanData.device = vkbDevice.device;
    vulkanData.chosenGPU = physicalDevice.physical_device;

    vulkanData.debug_messenger = vkb_inst.debug_messenger;

    // use vkbootstrap to get a Graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = vulkanData.chosenGPU;
    allocatorInfo.device = vulkanData.device;
    allocatorInfo.instance = vulkanData.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &vulkanData.allocator);

    vulkanData.mainDeletionQueue.push_function([&]()
                                               { vmaDestroyAllocator(vulkanData.allocator); });

    return 0;

    // spdlog::info("UFMOEngine::init vulkan finished");
}

uint8_t VulkanRenderer::init()
{
    ZoneScoped;
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

    vulkanData.window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        vulkanData.windowExtent.width,
        vulkanData.windowExtent.height,
        window_flags);

    initVulkan();

    initSwapchain();

    initCommands();

    initSyncStructures();

    // everything went fine
    _isInitialized = true;
    return 0;
    // spdlog::info("UFMOEngine::init finished");
}

void VulkanRenderer::run()
{
    ZoneScoped;
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
void VulkanRenderer::createSwapchain(uint32_t width, uint32_t height)
{
}

void VulkanRenderer::initSwapchain()
{
    ZoneScoped;
    if (!p_swapchain)
    {
        p_swapchain = std::make_unique<Swapchain>(vulkanData);
    }
    p_swapchain->initSwapchain();
}

void VulkanRenderer::destroySwapchain()
{
    ZoneScoped;
    // TODO: delegate destroy to swapchain class
    vkDestroySwapchainKHR(vulkanData.device, p_swapchain->getDataRef().swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < p_swapchain->getDataRef().swapchainImageViews.size(); i++)
    {

        vkDestroyImageView(vulkanData.device, p_swapchain->getDataRef().swapchainImageViews[i], nullptr);
    }
    p_swapchain.reset(nullptr);
}

void VulkanRenderer::initCommands()
{
    ZoneScoped;
    spdlog::info("UFMOEngine::init commands");
    // create a command pool for commands submitted to the graphics queue.
    // we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = _graphicsQueueFamily;

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {

        VK_CHECK(vkCreateCommandPool(vulkanData.device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = _frames[i]._commandPool;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VK_CHECK(vkAllocateCommandBuffers(vulkanData.device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }
}

void VulkanRenderer::initSyncStructures()
{
    ZoneScoped;
    spdlog::info("UFMOEngine::init sync structures");
    // create syncronization structures
    // one fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain
    // we want the fence to start signalled so we can wait on it on the first frame

    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateFence(vulkanData.device, &fenceCreateInfo, VK_NULL_HANDLE, &_frames[i]._renderFence));

        VK_CHECK(vkCreateSemaphore(vulkanData.device, &semaphoreCreateInfo, VK_NULL_HANDLE, &_frames[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(vulkanData.device, &semaphoreCreateInfo, VK_NULL_HANDLE, &_frames[i]._renderSemaphore));
    }
}

void VulkanRenderer::draw_background(VkCommandBuffer cmd)
{
    // make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = abs(sin(_frameNumber / 120.f));
    flash = 1.0;
    clearValue = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    // clear image
    vkCmdClearColorImage(cmd, vulkanData.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void VulkanRenderer::draw()
{
    ZoneScoped;
    // spdlog::debug("...{}",_frameNumber % FRAME_OVERLAP);

    // request image from the swapchain
    uint32_t swapchainImageIndex;

    // if ( int i = VK_TIMOUT );
    {
        ZoneScopedN("Wait for Fence");
        VK_CHECK(vkWaitForFences(vulkanData.device, 1, &get_current_frame()._renderFence, true, 1000000000));

        get_current_frame()._deletionQueue.flush();
        VK_CHECK(vkResetFences(vulkanData.device, 1, &get_current_frame()._renderFence));
    }

    {
        ZoneScopedN("Aquire Next Image");
        auto result = vkAcquireNextImageKHR(vulkanData.device, p_swapchain->getDataRef().swapchain, 1000000000, get_current_frame()._swapchainSemaphore, VK_NULL_HANDLE, &swapchainImageIndex);
    }

    //  wait until the gpu has finished rendering the last frame. Timeout of 1
    //  second

    // naming it cmd for shorter writing

    auto cmdPool = get_current_frame()._commandPool;

    {
        ZoneScopedN("Reset Command Pool");
        VK_CHECK(vkResetCommandPool(vulkanData.device, cmdPool, 0));
    }
    auto cmd = get_current_frame()._mainCommandBuffer;

    // now that we are sure that the commands finished executing, we can safely
    // reset the command buffer to begin recording again.
    // VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    {
        ZoneScopedN("Command Buffer");
        vulkanData.drawExtent.width = vulkanData.drawImage.imageExtent.width;
        vulkanData.drawExtent.height = vulkanData.drawImage.imageExtent.height;

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // transition our main draw image into general layout so we can write into it
        // we will overwrite it all so we dont care about what was the older layout
        vkutil::transition_image(cmd, vulkanData.drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        draw_background(cmd);

        // transition the draw image and the swapchain image into their correct transfer layouts
        vkutil::transition_image(cmd, vulkanData.drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transition_image(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // execute a copy from the draw image into the swapchain (blit)
        // VK_ACCESS_2_TRANSFER_READ_BIT specifies read access to an image or buffer in a copy operation. 
        // Such access occurs in the VK_PIPELINE_STAGE_2_COPY_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, or VK_PIPELINE_STAGE_2_RESOLVE_BIT pipeline stages.
        vkutil::copy_image_to_image(cmd, vulkanData.drawImage.image, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex],
                                    vulkanData.drawExtent, p_swapchain->getDataRef().swapchainExtent);

        // set swapchain image layout to Present so we can show it on the screen
         //vkutil::transition_image(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkutil::transition_image_to_present(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // finalize the command buffer (we can no longer add commands, but it can now be executed)
        VK_CHECK(vkEndCommandBuffer(cmd));

        /*         ZoneScopedN("Command Buffer");
                // start the command buffer recording
                VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

                // make the swapchain image into writeable mode before rendering
                vkutil::transition_image(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

                // make a clear-color from frame number. This will flash with a 120 frame period.
                VkClearColorValue clearValue;
                float flash = abs(sin(_frameNumber / 120.f));
                flash = 1.0f;
                // if (_frameNumber % 120 )

                clearValue = {{0.0f, 0.0f, flash, 1.0f}};

                VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

                // clear image
                vkCmdClearColorImage(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

                // make the swapchain image into presentable mode VK_IMAGE_LAYOUT_GENERAL  VK_IMAGE_LAYOUT_UNDEFINED
                // Write after Write
                vkutil::transition_image_to_present(cmd, p_swapchain->getDataRef().swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

                // finalize the command buffer (we can no longer add commands, but it can now be executed)
                VK_CHECK(vkEndCommandBuffer(cmd)); */
    }

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    {
        ZoneScopedN("Submit");
        VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));
    }

    // prepare present
    // this will put the image we just rendered to into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &p_swapchain->getDataRef().swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    // Present after Write
    {
        ZoneScopedN("Present");
        VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
    }

    // increase the number of frames drawn
    _frameNumber++;

    FrameMark;

    // if (result == VK_TIMEOUT)

    // VK_CHECK(vkAcquireNextImageKHR(_device, swapchain.swapchain, 1000000000, get_current_frame()._swapchainSemaphore,  VK_NULL_HANDLE, &swapchainImageIndex));
}
