#include "Application.hpp"

#include "Config.hpp"
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <iostream>
#include <set>

using namespace std;

bool Application::isDeviceSuitable(const vk::PhysicalDevice& device) const
{
  vk::PhysicalDeviceProperties props = device.getProperties();
  vector<vk::ExtensionProperties> device_extensions = device.enumerateDeviceExtensionProperties();

  // Return false if physical device is not a discrete GPU
  if (props.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
    return false;

  // Create a list of potentially unsupported device extensions
  set<string> unsupported_extensions(
        this->requested_device_extensions_.cbegin(),
        this->requested_device_extensions_.cend());

  // Eliminate supported extensions from the unsupported extensions set
  for (const auto& device_extension : device_extensions)
    unsupported_extensions.erase(string(device_extension.extensionName));

  // Return false if there are unsupported device extensions
  if (!unsupported_extensions.empty())
    return false;

  auto swapchain_support = querySwapchainSupportDetails(device, this->surface_);
  if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty())
    return false;

  return true;
}

Application::QueueFamilyIndices Application::findQueueFamilies(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& surface) const
{
  QueueFamilyIndices queue_family_indices;
  auto queue_families_properties = device.getQueueFamilyProperties();
  for (uint32_t i = 0; i < queue_families_properties.size(); i++)
    {
      // Get the current queue family properties
      vk::QueueFamilyProperties &prop = queue_families_properties.at(i);
      // Check queue family capabilities for graphics, transfer and compute
      if (prop.queueFlags & vk::QueueFlagBits::eGraphics)
          queue_family_indices.graphics = i;
      if (prop.queueFlags & vk::QueueFlagBits::eTransfer)
          queue_family_indices.transfer = i;
      if (prop.queueFlags & vk::QueueFlagBits::eCompute)
          queue_family_indices.compute = i;
      // Check queue family capabilities for presentation support
      if (device.getSurfaceSupportKHR(i, surface) == VK_TRUE)
          queue_family_indices.present = i;
    }
  return queue_family_indices;
}

Application::SwapchainSupportDetails Application::querySwapchainSupportDetails(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& surface) const
{
  Application::SwapchainSupportDetails swapchain_support;
  swapchain_support.capabilities = device.getSurfaceCapabilitiesKHR(surface);
  swapchain_support.formats = device.getSurfaceFormatsKHR(surface);
  swapchain_support.present_modes = device.getSurfacePresentModesKHR(surface);
  return swapchain_support;
}

vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(
    const vector<vk::SurfaceFormatKHR>& available_formats) const
{
  auto result = find_if(available_formats.cbegin(), available_formats.cend(), [](vk::SurfaceFormatKHR i) {
      return i.format == vk::Format::eB8G8R8A8Srgb && i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
  if (result != available_formats.cend())
    return *result;
  return available_formats.front();
}

vk::PresentModeKHR Application::chooseSwapPresentMode(
    const vector<vk::PresentModeKHR>& available_present_modes) const
{
  for (auto available_present_mode : available_present_modes)
      if (available_present_mode == vk::PresentModeKHR::eMailbox)
        return available_present_mode;
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Application::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities) const
{
  if (capabilities.currentExtent.width != UINT32_MAX &&
      capabilities.currentExtent.height != UINT32_MAX)
    {
      return capabilities.currentExtent;
    }
  vk::Extent2D extent = {800, 600};
  extent.width = clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
  extent.height = clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);
  return extent;
}

void Application::initSDL()
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    throw runtime_error(SDL_GetError());
  this->window_ = SDL_CreateWindow(
        "Vulkan-Engine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_VULKAN);
  if (!this->window_)
    throw runtime_error(SDL_GetError());
}

void Application::initInstance()
{
  // Get required extensions for SDL
  uint32_t sdl_extension_count = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window_, &sdl_extension_count, nullptr);
  if (sdl_extension_count > 0)
    {
      // Resize the requested extensions array to accommodate our insertions
      size_t requested_extensions_offset = this->requested_instance_extensions_.size();
      this->requested_instance_extensions_.resize(requested_extensions_offset + sdl_extension_count);
      SDL_Vulkan_GetInstanceExtensions(this->window_, &sdl_extension_count, this->requested_instance_extensions_.data() + requested_extensions_offset);
    }

  // Make the application version
  uint32_t app_version = VK_MAKE_VERSION(
        PROJECT_VERSION_MAJOR,
        PROJECT_VERSION_MINOR,
        PROJECT_VERSION_PATCH);

  // Make the engine version
  uint32_t engine_version = VK_MAKE_VERSION(
        PROJECT_VERSION_MAJOR,
        PROJECT_VERSION_MINOR,
        PROJECT_VERSION_PATCH);

  // Initialise application information
  vk::ApplicationInfo app_info(
        "VulkanEngine",
        app_version,
        "No Engine",
        engine_version,
        VK_API_VERSION_1_0);

  // Initialise instance create information
  vk::InstanceCreateInfo create_info(
        vk::InstanceCreateFlags{},
        &app_info,
        this->requested_instance_layers_.size(),
        this->requested_instance_layers_.data(),
        this->requested_instance_extensions_.size(),
        this->requested_instance_extensions_.data());

  // Create Vulkan instance
  if (vk::createInstance(&create_info, nullptr, &this->instance_) != vk::Result::eSuccess)
    throw runtime_error("Failed to initialise Vulkan instance");

  // Create Vulkan surface using SDL
  VkSurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(this->window_, this->instance_, &surface))
    throw runtime_error("Failed to create a Vulkan surface");

  this->surface_ = vk::SurfaceKHR(surface);
}

void Application::initPhysicalDevice()
{
  vector<vk::PhysicalDevice> physical_devices = this->instance_.enumeratePhysicalDevices();
  if (physical_devices.size() == 0)
    throw runtime_error("Unable to find Vulkan compatible device");
  cout << "Found " << physical_devices.size() << " Vulkan compatible device(s)" << endl;
  // Iterate all discovered physical devices and get the first suitable one
  for (vk::PhysicalDevice physical_device : physical_devices)
    {
      if (this->isDeviceSuitable(physical_device))
        {
          this->physical_device_ = physical_device;
          break;
        }
    }
  // Check if no suitable physical device could be found
  if (!this->physical_device_)
    throw runtime_error("Unable to find Vulkan compatible discrete GPU");
  cout << "Device selected: " << this->physical_device_.getProperties().deviceName << endl;
}

void Application::initQueueFamilies()
{
  // Get all the queue family indices for a physical device
  this->queue_family_indices_ = this->findQueueFamilies(this->physical_device_, this->surface_);
  // Check if required queues exist
  if (!this->queue_family_indices_.graphics.has_value())
    throw runtime_error("Could not find a graphics queue for selected device");
  if (!this->queue_family_indices_.present.has_value())
    throw runtime_error("Could not find a present queue for selected device");
  // Check if optional queues exist
  if (!this->queue_family_indices_.transfer.has_value())
    cerr << "Could not find a transfer queue for selected device" << endl;
  if (!this->queue_family_indices_.compute.has_value())
    cerr << "Could not find a compute queue for selected device" << endl;
}

void Application::initDevice()
{
  // Gather all queue family indices into an iterable set
  set<uint32_t> queue_family_set{};
  queue_family_set.insert(queue_family_indices_.graphics.value());
  queue_family_set.insert(queue_family_indices_.transfer.value());
  queue_family_set.insert(queue_family_indices_.compute.value());
  queue_family_set.insert(queue_family_indices_.present.value());

  // Prepare queue create structures
  vector<vk::DeviceQueueCreateInfo> queue_create_infos{};
  float queue_priority = 1.0f;
  for (uint32_t queue_family : queue_family_set)
    {
      vk::DeviceQueueCreateInfo queue_create_info(
            vk::DeviceQueueCreateFlags{},
            queue_family, 1, &queue_priority);
      queue_create_infos.push_back(queue_create_info);
    }

  // Prepare enabled physical device features
  vk::PhysicalDeviceFeatures requested_device_features = {};

  // Prepare the logical device create structure
  vk::DeviceCreateInfo create_info(
        vk::DeviceCreateFlags{},
        queue_create_infos.size(),
        queue_create_infos.data(),
        this->requested_device_layers_.size(),
        this->requested_device_layers_.data(),
        this->requested_device_extensions_.size(),
        this->requested_device_extensions_.data(),
        &requested_device_features);

  this->device_ = this->physical_device_.createDevice(create_info);
  if (!this->device_)
    throw runtime_error("Unable to create logical device");

  queues_.graphics = this->device_.getQueue(queue_family_indices_.graphics.value(), 0);
  queues_.transfer = this->device_.getQueue(queue_family_indices_.transfer.value(), 0);
  queues_.compute = this->device_.getQueue(queue_family_indices_.compute.value(), 0);
  queues_.present = this->device_.getQueue(queue_family_indices_.present.value(), 0);
}

void Application::initSwapchain()
{
  SwapchainSupportDetails swapchain_support = querySwapchainSupportDetails(
        this->physical_device_, this->surface_);

  vk::SurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
  vk::PresentModeKHR present_mode = chooseSwapPresentMode(swapchain_support.present_modes);
  vk::Extent2D extent = chooseSwapExtent(swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
  uint32_t image_layers = 1;

  // Ensure that image_count is no higher than our maximum image count
  if (swapchain_support.capabilities.maxImageCount > 0) {
      image_count = clamp(
            image_count,
            swapchain_support.capabilities.minImageCount,
            swapchain_support.capabilities.maxImageCount);
  }

  vk::SharingMode image_sharing_mode;
  vector<uint32_t> queue_family_indices;
  if (this->queue_family_indices_.graphics.value() !=
      this->queue_family_indices_.present.value())
    {
      image_sharing_mode = vk::SharingMode::eConcurrent;
      queue_family_indices = {
        this->queue_family_indices_.graphics.value(),
        this->queue_family_indices_.present.value()
      };
    }
  else
    {
      image_sharing_mode = vk::SharingMode::eExclusive;
    }

  vk::SwapchainCreateInfoKHR create_info(
        vk::SwapchainCreateFlagsKHR{},
        this->surface_,
        image_count,
        surface_format.format,
        surface_format.colorSpace,
        extent,
        image_layers,
        vk::ImageUsageFlagBits::eColorAttachment,
        image_sharing_mode,
        queue_family_indices.size(),
        queue_family_indices.data(),
        swapchain_support.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode,
        VK_TRUE,
        nullptr);

  this->swapchain_ = this->device_.createSwapchainKHR(create_info);
  this->swapchain_images_ = this->device_.getSwapchainImagesKHR(this->swapchain_);
}

void Application::init()
{
  this->initSDL();
  this->initInstance();
  this->initPhysicalDevice();
  this->initQueueFamilies();
  this->initDevice();
  this->initSwapchain();
}

void Application::main()
{
}

void Application::cleanup()
{
  // Vulkan C++ SurfaceKHR wrapper does not have a destroy method.
  vkDestroySwapchainKHR(this->device_, this->swapchain_, nullptr);
  vkDestroySurfaceKHR(this->instance_, this->surface_, nullptr);
  this->device_.destroy();
  this->instance_.destroy();
}

void Application::run()
{
  this->init();
  this->main();
  this->cleanup();
}
