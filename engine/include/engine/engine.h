#pragma once

#include "vk_types.h"
#include <memory>
// #define _NO_DEBUG_HEAP 1

struct FrameData
{

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;
};

constexpr unsigned int FRAME_OVERLAP = 3;

struct AllocatorCallback {
    // TODO: allocator callback implementation    
    static VkAllocationCallbacks *p_allocatorCallback;
};

//TODO: rename to VulkanContext?
struct BasicVulkanData {
    VkInstance instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice chosenGPU;               // GPU chosen as the default device
    VkDevice device;                          // Vulkan device for commands
    VkSurfaceKHR surface;                     // Vulkan window surface
    struct SDL_Window* window{nullptr};
    VkExtent2D windowExtent{1700, 900};
};


struct SwapchainData{
    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent;
};

class Swapchain
{
public:
    Swapchain(const BasicVulkanData& vulkanData);// : m_vulkanData(vulkanData){ };
    ~Swapchain();

    void init_swapchain();    
    void create_swapchain(uint32_t width, uint32_t height);
    SwapchainData& getDataRef() {return m_data;};
private:
    

    const BasicVulkanData& m_vulkanData;
    SwapchainData m_data;

    //VkSwapchainKHR swapchain;
    //VkFormat swapchainImageFormat;
    //std::vector<VkImage> swapchainImages;
    //std::vector<VkImageView> swapchainImageViews;
    //VkExtent2D swapchainExtent;
};



class UFMOEngine
{
private:
    

    bool _isInitialized{false};
    int _frameNumber{0};
    bool stop_rendering{false};
    //VkExtent2D _windowExtent{1700, 900};

    // instance + device
    BasicVulkanData vulkanData;
    //VkInstance _instance;                      // Vulkan library handle
    //VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    //VkPhysicalDevice _chosenGPU;               // GPU chosen as the default device
    //VkDevice _device;                          // Vulkan device for commands
    //VkSurfaceKHR _surface;                     // Vulkan window surface

    // Swapchain
    std::unique_ptr<Swapchain> p_swapchain;
    // VkSwapchainKHR _swapchain;
    // VkFormat _swapchainImageFormat;
    // std::vector<VkImage> _swapchainImages;
    // std::vector<VkImageView> _swapchainImageViews;
    // VkExtent2D _swapchainExtent;

    //struct SDL_Window *_window{nullptr};

    FrameData _frames[FRAME_OVERLAP];

    FrameData &get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

public:
    UFMOEngine &get();
    uint8_t init();
    uint8_t init_vulkan();
    void run();
    void cleanup();
    void create_swapchain(uint32_t width, uint32_t height);
    void init_swapchain();
    void destroy_swapchain();
    void init_commands();

    void init_sync_structures();
    void draw();
};