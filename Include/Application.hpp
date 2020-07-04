#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <SDL2/SDL.h>
#include <optional>
#include <vulkan/vulkan.hpp>

class Application
{
private:
  // Required layers and extensions for the Vulkan instance and devices
  std::vector<const char*> required_instance_layers_     = { "VK_LAYER_KHRONOS_validation" };
  std::vector<const char*> required_instance_extensions_ = {};
  std::vector<const char*> required_device_layers_       = {};
  std::vector<const char*> required_device_extensions_   = { "VK_KHR_swapchain" };

  // Window dimensions
  const uint32_t window_width_  = 800;
  const uint32_t window_height_ = 600;

  // SDL window handle
  SDL_Window* window_;

  // Vulkan instance
  vk::Instance instance_;

  // Vulkan physical device
  vk::PhysicalDevice physical_device_;

  // Vulkan logical device
  vk::Device device_;

  // Vulkan surface
  vk::SurfaceKHR surface_;

  // Vulkan swapchain
  vk::SwapchainKHR swapchain_;
  std::vector<vk::Image> swapchain_images_;
  std::vector<vk::ImageView> swapchain_image_views_;
  vk::Format swapchain_format_;
  vk::Extent2D swapchain_extent_;

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> transfer;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> present;
  } queue_family_indices_;

  struct Queues
  {
    vk::Queue graphics;
    vk::Queue transfer;
    vk::Queue compute;
    vk::Queue present;
  } queues_;

  struct SwapchainSupportDetails
  {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
  } swapchain_support_details_;

  // readFile reads an entire file and returns it as a vector of chars
  std::vector<char> readFile(const std::string& file) const;

  // isDeviceSuitable takes a physical device and returns true if it is a suitable device, it will
  // return false if the device is not suitable.
  bool isDeviceSuitable(const vk::PhysicalDevice& phys_dev) const;

  // Returns a QueueFamilyIndices struct for the physical device
  QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& phys_dev) const;

  // Queries a physical device for swapchain support details with a surface
  SwapchainSupportDetails querySwapchainSupportDetails(const vk::PhysicalDevice& phys_dev) const;

  vk::SurfaceFormatKHR
  chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) const;

  vk::PresentModeKHR
  chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) const;

  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

  vk::ShaderModule createShaderModule(const std::vector<char>& shader_code);

  // Initialises the SDL library and a native window
  void initSDL();

  // Initialises the Vulkan instance and SDL Vulkan surface
  void initInstance();

  // Initialises the physical device
  void initPhysicalDevice();

  // Initialises queue families for the physical device
  void initQueueFamilies();

  // Initialises the logical device and queues
  void initDevice();

  // Initialises the swapchain
  void initSwapchain();

  // Initialises the swapchain image views
  void initSwapchainImageViews();

  // Initialises the graphics pipeline
  void initGraphicsPipeline();

public:
  Application();

  void run();

  ~Application();
};

#endif
