#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <optional>

class Application
{
private:
  std::vector<const char*> requested_instance_layers_ = {"VK_LAYER_KHRONOS_validation"};
  std::vector<const char*> requested_instance_extensions_ = {};
  std::vector<const char*> requested_device_layers_ = {};
  std::vector<const char*> requested_device_extensions_ = {"VK_KHR_swapchain"};

  // SDL Window
  SDL_Window *window_;

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

  // Vulkan swapchain images
  std::vector<vk::Image> swapchain_images_;

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> transfer;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> present;
  } queue_family_indices_;

  struct Queues {
    vk::Queue graphics;
    vk::Queue transfer;
    vk::Queue compute;
    vk::Queue present;
  } queues_;

  struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
  } swapchain_support_details_;

  // Returns if a physical device is suitable to be used
  bool isDeviceSuitable(const vk::PhysicalDevice& device) const;

  // Returns a QueueFamilyIndices struct for the physical device
  QueueFamilyIndices findQueueFamilies(
      const vk::PhysicalDevice& device,
      const vk::SurfaceKHR& surface) const;

  // Queries a physical device for swapchain support details with a surface
  SwapchainSupportDetails querySwapchainSupportDetails(
      const vk::PhysicalDevice& physical_device,
      const vk::SurfaceKHR& surface) const;

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR>& available_formats) const;

  vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR>& available_present_modes) const;

  vk::Extent2D chooseSwapExtent(
      const vk::SurfaceCapabilitiesKHR& capabilities) const;

  // Initialises the SDL library and creates a window
  void initSDL();

  // Initialises the Vulkan runtime and surfaces
  void initInstance();

  // Initialises the physical device
  void initPhysicalDevice();

  // Initialises queue families for the physical device
  void initQueueFamilies();

  // Initialises the logical device and queues
  void initDevice();

  // Initialises the swap chain
  void initSwapchain();

  void init();

  void main();

  void cleanup();
public:
  void run();
};

#endif
