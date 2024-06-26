#pragma once

#include "vk_types.h"
#include "engine/vk_descriptors.h"
#include "../src/vk_pipelines.h"
//#include <memory>
//#include <tracy/Tracy.hpp>

class VulkanEngine{

    VmaAllocator _allocator;
};

// #define _NO_DEBUG_HEAP 1

// TODO: because we are storing whole std::functions for every object we are deleting,
// which is not going to be optimal. For the amount of objects we will use in this tutorial,
//  its going to be fine. but if you need to delete thousands of objects and want them deleted faster,
// a better implementation would be to store arrays of vulkan handles of various types such as VkImage,
//   VkBuffer, and so on. And then delete those from a loop.
struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        ZoneScoped;
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)(); // call functors
        }

        deletors.clear();
    }
};

struct FrameData
{

    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;

    VkSemaphore _swapchainSemaphore, _renderSemaphore;
    VkFence _renderFence;
    DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 3;

struct AllocatorCallback {
    // TODO: allocator callback implementation    
    static VkAllocationCallbacks *p_allocatorCallback;
};

//TODO: rename to VulkanContext / Device?
// Device / Swapchain (with Images) / Surface
struct BasicVulkanData {
    VkInstance instance;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice chosenGPU;               // GPU chosen as the default device
    VkDevice device;                          // Vulkan device for commands
    VkSurfaceKHR surface;                     // Vulkan window surface
    struct SDL_Window* window{nullptr};
    VkExtent2D windowExtent{1700, 900};
    VmaAllocator allocator;
    //draw resources
	AllocatedImage drawImage;
	VkExtent2D drawExtent;
    DeletionQueue mainDeletionQueue;
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
    Swapchain(BasicVulkanData& vulkanData);// : m_vulkanData(vulkanData){ };
    ~Swapchain();

    void initSwapchain();    
    void createSwapchain(uint32_t width, uint32_t height);
    SwapchainData& getDataRef() {return m_data;};

    
	
private:
    
    //TODO: better modularisationb and naming
    BasicVulkanData& m_vulkanData;
    SwapchainData m_data;

    //VkSwapchainKHR swapchain;
    //VkFormat swapchainImageFormat;
    //std::vector<VkImage> swapchainImages;
    //std::vector<VkImageView> swapchainImageViews;
    //VkExtent2D swapchainExtent;
};

class UFMOEngine;

class VulkanRenderer
{
private:
    
    //VmaAllocator _allocator;
    bool _isInitialized{false};
    int _frameNumber{0};
    bool stop_rendering{false};    

    // instance + device
    BasicVulkanData vulkanData;
    
    // Swapchain
    std::unique_ptr<Swapchain> p_swapchain;
    
    

    FrameData _frames[FRAME_OVERLAP];

    FrameData &get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;
    void init_descriptors();

public:
    //immidiate
    // immediate submit structures
    VkFence _immFence;
    VkCommandBuffer _immCommandBuffer;
    VkCommandPool _immCommandPool;

	
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    void init_imgui();

    DescriptorAllocator globalDescriptorAllocator;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;
    	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

    VulkanRenderer &get();
    uint8_t init();
    uint8_t initVulkan();
    void run();
    void tearDown();
    void createSwapchain(uint32_t width, uint32_t height);
    void initSwapchain();
    void destroySwapchain();
    void initCommands();

    void initSyncStructures();
    void draw();
    void draw_background(VkCommandBuffer cmd);
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
    void init_pipelines();
	void init_background_pipelines();
};