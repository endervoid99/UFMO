#pragma once

#include "vk_types.h"
// #define _NO_DEBUG_HEAP 1

class UFMOEngine
{
private:
    bool _isInitialized{false};
    int _frameNumber{0};
    bool stop_rendering{false};
    VkExtent2D _windowExtent{1700, 900};

    //instance + device
    VkInstance _instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU;               // GPU chosen as the default device
    VkDevice _device;                          // Vulkan device for commands
    VkSurfaceKHR _surface;                     // Vulkan window surface

    //Swapchain
    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;

    struct SDL_Window *_window{nullptr};

public:
    UFMOEngine &get();
    void init();
    void init_vulkan();
    void run();
    void cleanup();
    void create_swapchain(uint32_t width, uint32_t height);
    void init_swapchain();
    void destroy_swapchain();
};