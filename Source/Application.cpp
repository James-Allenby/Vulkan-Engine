#include "Application.hpp"

#include "Config.hpp"

#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

std::vector<char> Application::readFile(const std::string& file_name) const
{
  // Open file to read
  std::ifstream file(file_name, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open file");
  // Get the size of the file
  auto file_size = file.tellg();
  // Allocate a vector the size of the file
  std::vector<char> file_data(file_size);
  // Read the file into the file data
  file.seekg(0);
  file.read(file_data.data(), file_size);
  file.close();
  return file_data;
}

bool Application::isDeviceSuitable(const vk::PhysicalDevice& phys_dev) const
{
  vk::PhysicalDeviceProperties pdev_props        = phys_dev.getProperties();
  std::vector<vk::ExtensionProperties> pdev_exts = phys_dev.enumerateDeviceExtensionProperties();

  // Check if physical device is not a discrete GPU
  if (pdev_props.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
    return false;

  // Create a set of requested physical device extensions
  std::set<std::string> unsupported_extensions(this->required_device_extensions_.cbegin(),
                                               this->required_device_extensions_.cend());

  // Delete items from unsupported extensions if they are supported
  for (const auto& pdev_ext : pdev_exts)
    unsupported_extensions.erase(std::string(pdev_ext.extensionName));

  // Check if there are any unsupported extensions remaining
  if (!unsupported_extensions.empty())
    return false;

  // Check swapchain support for physical device
  auto swapchain_support = querySwapchainSupportDetails(phys_dev);
  if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty())
    return false;

  return true;
}

Application::QueueFamilyIndices
Application::findQueueFamilies(const vk::PhysicalDevice& phys_dev) const
{
  QueueFamilyIndices queue_family_indices;
  auto queue_families_properties = phys_dev.getQueueFamilyProperties();
  for (uint32_t i = 0; i < queue_families_properties.size(); i++)
  {
    // Get the current queue family properties
    vk::QueueFamilyProperties& prop = queue_families_properties.at(i);

    // Check queue family capabilities for graphics, transfer and compute
    if (prop.queueFlags & vk::QueueFlagBits::eGraphics)
      queue_family_indices.graphics = i;
    if (prop.queueFlags & vk::QueueFlagBits::eTransfer)
      queue_family_indices.transfer = i;
    if (prop.queueFlags & vk::QueueFlagBits::eCompute)
      queue_family_indices.compute = i;

    // Check queue family capabilities for presentation support
    if (phys_dev.getSurfaceSupportKHR(i, this->surface_) == VK_TRUE)
      queue_family_indices.present = i;
  }
  return queue_family_indices;
}

Application::SwapchainSupportDetails
Application::querySwapchainSupportDetails(const vk::PhysicalDevice& phys_dev) const
{
  Application::SwapchainSupportDetails swapchain_support;
  swapchain_support.capabilities  = phys_dev.getSurfaceCapabilitiesKHR(this->surface_);
  swapchain_support.formats       = phys_dev.getSurfaceFormatsKHR(this->surface_);
  swapchain_support.present_modes = phys_dev.getSurfacePresentModesKHR(this->surface_);
  return swapchain_support;
}

vk::SurfaceFormatKHR
Application::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const
{
  auto result = find_if(formats.cbegin(), formats.cend(), [](vk::SurfaceFormatKHR i) {
    return i.format == vk::Format::eB8G8R8A8Srgb &&
           i.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
  });
  if (result != formats.cend())
    return *result;
  return formats.front();
}

vk::PresentModeKHR
Application::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& present_modes) const
{
  for (auto available_present_mode : present_modes)
    if (available_present_mode == vk::PresentModeKHR::eMailbox)
      return available_present_mode;
  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
  if (capabilities.currentExtent.width != UINT32_MAX &&
      capabilities.currentExtent.height != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  vk::Extent2D extent = { this->window_width_, this->window_height_ };

  extent.width  = std::clamp(extent.width,
                            capabilities.minImageExtent.width,
                            capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height,
                             capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
  return extent;
}

vk::ShaderModule Application::createShaderModule(const std::vector<char>& shader_code)
{
  vk::ShaderModuleCreateInfo create_info;
  create_info.setCodeSize(shader_code.size())
      .setPCode(reinterpret_cast<const uint32_t*>(shader_code.data()));
  return this->device_.createShaderModule(create_info);
}

void Application::initSDL()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    throw std::runtime_error(SDL_GetError());
  this->window_ = SDL_CreateWindow("Vulkan-Engine",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   this->window_width_,
                                   this->window_height_,
                                   SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
  if (!this->window_)
    throw std::runtime_error(SDL_GetError());
}

void Application::initInstance()
{
  // Retrieve the count of required instance extensions from the SDL library
  uint32_t sdl_extension_count = 0;
  SDL_Vulkan_GetInstanceExtensions(this->window_, &sdl_extension_count, nullptr);
  if (sdl_extension_count > 0)
  {
    // Resize the requested extensions array to accommodate our insertions
    size_t requested_extensions_offset = this->required_instance_extensions_.size();
    this->required_instance_extensions_.resize(requested_extensions_offset + sdl_extension_count);
    SDL_Vulkan_GetInstanceExtensions(this->window_,
                                     &sdl_extension_count,
                                     this->required_instance_extensions_.data() +
                                         requested_extensions_offset);
  }

  // Make the application version
  uint32_t app_version =
      VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);

  // Make the engine version
  uint32_t engine_version =
      VK_MAKE_VERSION(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);

  // Initialise application information
  vk::ApplicationInfo app_info("VulkanEngine",
                               app_version,
                               "No Engine",
                               engine_version,
                               VK_API_VERSION_1_0);

  // Initialise instance create information
  vk::InstanceCreateInfo create_info(
      vk::InstanceCreateFlags {},
      &app_info,
      static_cast<uint32_t>(this->required_instance_layers_.size()),
      this->required_instance_layers_.data(),
      static_cast<uint32_t>(this->required_instance_extensions_.size()),
      this->required_instance_extensions_.data());

  // Create Vulkan instance
  this->instance_ = vk::createInstance(create_info);

  // Create Vulkan surface using SDL
  VkSurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(this->window_, this->instance_, &surface))
    throw std::runtime_error("Failed to create a Vulkan surface");

  this->surface_ = vk::SurfaceKHR(surface);
}

void Application::initPhysicalDevice()
{
  std::vector<vk::PhysicalDevice> phys_devs = this->instance_.enumeratePhysicalDevices();

  // Check if no devices could be found
  if (phys_devs.size() == 0)
    throw std::runtime_error("Unable to find Vulkan compatible device");
  std::cout << "Found " << phys_devs.size() << " Vulkan compatible device(s)" << std::endl;

  // Iterate through all physical devices and find first suitable one
  auto phys_dev = std::find_if(phys_devs.cbegin(), phys_devs.cend(), [&](auto& phys_dev) {
    return isDeviceSuitable(phys_dev);
  });

  // Check if no suitable physical device could be found
  if (phys_dev == phys_devs.cend())
    throw std::runtime_error("Unable to find Vulkan compatible discrete GPU");

  this->physical_device_ = *phys_dev;
  std::cout << "Device selected: " << this->physical_device_.getProperties().deviceName
            << std::endl;
}

void Application::initQueueFamilies()
{
  // Get all the queue family indices for a physical device
  this->queue_family_indices_ = this->findQueueFamilies(this->physical_device_);

  // Check if required queues exist
  if (!this->queue_family_indices_.graphics.has_value())
    throw std::runtime_error("Could not find a graphics queue for selected device");
  if (!this->queue_family_indices_.present.has_value())
    throw std::runtime_error("Could not find a present queue for selected device");

  // Check if optional queues exist
  if (!this->queue_family_indices_.transfer.has_value())
    std::cerr << "Could not find a transfer queue for selected device" << std::endl;
  if (!this->queue_family_indices_.compute.has_value())
    std::cerr << "Could not find a compute queue for selected device" << std::endl;
}

void Application::initDevice()
{
  // Gather all queue family indices into an iterable set
  std::set<uint32_t> queue_family_set { this->queue_family_indices_.graphics.value(),
                                        this->queue_family_indices_.transfer.value(),
                                        this->queue_family_indices_.compute.value(),
                                        this->queue_family_indices_.present.value() };

  // Prepare to create queues for all indices
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos = {};
  float queue_priority                                      = 1.0f;
  uint32_t queue_count                                      = 1;
  for (auto& queue_family : queue_family_set)
  {
    vk::DeviceQueueCreateInfo queue_create_info(vk::DeviceQueueCreateFlags {},
                                                queue_family,
                                                queue_count,
                                                &queue_priority);
    queue_create_infos.push_back(queue_create_info);
  }

  // Prepare enabled physical device features
  vk::PhysicalDeviceFeatures requested_device_features = {};

  // Prepare the logical device create structure
  vk::DeviceCreateInfo create_info(vk::DeviceCreateFlags {},
                                   queue_create_infos.size(),
                                   queue_create_infos.data(),
                                   static_cast<uint32_t>(this->required_device_layers_.size()),
                                   this->required_device_layers_.data(),
                                   static_cast<uint32_t>(this->required_device_extensions_.size()),
                                   this->required_device_extensions_.data(),
                                   &requested_device_features);

  this->device_ = this->physical_device_.createDevice(create_info);

  // Get the first queue for each type of queue
  queues_.graphics = this->device_.getQueue(queue_family_indices_.graphics.value(), 0);
  queues_.transfer = this->device_.getQueue(queue_family_indices_.transfer.value(), 0);
  queues_.compute  = this->device_.getQueue(queue_family_indices_.compute.value(), 0);
  queues_.present  = this->device_.getQueue(queue_family_indices_.present.value(), 0);
}

void Application::initSwapchain()
{
  SwapchainSupportDetails swapchain_support =
      this->querySwapchainSupportDetails(this->physical_device_);

  vk::SurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
  vk::PresentModeKHR present_mode     = chooseSwapPresentMode(swapchain_support.present_modes);
  vk::Extent2D extent                 = chooseSwapExtent(swapchain_support.capabilities);

  uint32_t image_count  = swapchain_support.capabilities.minImageCount + 1;
  uint32_t image_layers = 1;

  // Ensure that image_count is no higher than our maximum image count
  if (swapchain_support.capabilities.maxImageCount > 0)
  {
    image_count = std::clamp(image_count,
                             swapchain_support.capabilities.minImageCount,
                             swapchain_support.capabilities.maxImageCount);
  }

  vk::SharingMode image_sharing_mode;
  std::vector<uint32_t> queue_family_indices;
  if (this->queue_family_indices_.graphics.value() != this->queue_family_indices_.present.value())
  {
    image_sharing_mode   = vk::SharingMode::eConcurrent;
    queue_family_indices = { this->queue_family_indices_.graphics.value(),
                             this->queue_family_indices_.present.value() };
  } else
  {
    image_sharing_mode = vk::SharingMode::eExclusive;
  }

  vk::SwapchainCreateInfoKHR create_info;
  create_info.setFlags(vk::SwapchainCreateFlagsKHR {})
      .setSurface(this->surface_)
      .setMinImageCount(image_count)
      .setImageFormat(surface_format.format)
      .setImageColorSpace(surface_format.colorSpace)
      .setImageExtent(extent)
      .setImageArrayLayers(image_layers)
      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
      .setImageSharingMode(image_sharing_mode)
      .setQueueFamilyIndexCount(queue_family_indices.size())
      .setPQueueFamilyIndices(queue_family_indices.data())
      .setPreTransform(swapchain_support.capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(present_mode)
      .setClipped(VK_TRUE)
      .setOldSwapchain(nullptr);

  this->swapchain_        = this->device_.createSwapchainKHR(create_info);
  this->swapchain_images_ = this->device_.getSwapchainImagesKHR(this->swapchain_);
  this->swapchain_format_ = surface_format.format;
  this->swapchain_extent_ = extent;
}

void Application::initSwapchainImageViews()
{
  // Prepare the image view component map
  vk::ComponentMapping component_map;
  component_map.setR(vk::ComponentSwizzle::eIdentity)
      .setG(vk::ComponentSwizzle::eIdentity)
      .setB(vk::ComponentSwizzle::eIdentity)
      .setA(vk::ComponentSwizzle::eIdentity);

  // Prepare the image view subresource range
  vk::ImageSubresourceRange subresource_range;
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

  // Interate through all swapchain images
  for (auto& swapchain_image : this->swapchain_images_)
  {
    // Prepare the image view create info
    vk::ImageViewCreateInfo create_info;
    create_info.setFlags(vk::ImageViewCreateFlags {})
        .setImage(swapchain_image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(this->swapchain_format_)
        .setComponents(component_map)
        .setSubresourceRange(subresource_range);

    // Create the image view and push it into the swapchain image views vector
    vk::ImageView image_view = this->device_.createImageView(create_info);
    this->swapchain_image_views_.push_back(image_view);
  }
}

void Application::initGraphicsPipeline()
{
  // Load SPIR-V code from file into memory
  auto vert_shader_code = this->readFile("vert.spv");
  auto frag_shader_code = this->readFile("frag.spv");

  // Create Vulkan shader modules from loaded SPIR-V code
  vk::ShaderModule vert_shader_module = this->createShaderModule(vert_shader_code);
  vk::ShaderModule frag_shader_module = this->createShaderModule(frag_shader_code);

  // Prepare the vertex shader stage create info
  vk::PipelineShaderStageCreateInfo vert_shader_stage_ci;
  vert_shader_stage_ci.setStage(vk::ShaderStageFlagBits::eVertex)
      .setModule(vert_shader_module)
      .setPName("main");

  // Prepare the fragment shader stage create info
  vk::PipelineShaderStageCreateInfo frag_shader_stage_ci;
  frag_shader_stage_ci.setStage(vk::ShaderStageFlagBits::eFragment)
      .setModule(frag_shader_module)
      .setPName("main");

  // Collate shader stages create informations
  std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = { vert_shader_stage_ci,
                                                                   frag_shader_stage_ci };

  vk::PipelineVertexInputStateCreateInfo vert_input_state_ci;
  vert_input_state_ci.setVertexAttributeDescriptionCount(0).setVertexBindingDescriptionCount(0);

  vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_ci;
  input_assembly_state_ci.setTopology(vk::PrimitiveTopology::eTriangleList)
      .setPrimitiveRestartEnable(VK_FALSE);

  vk::Viewport viewport;
  viewport.setX(0)
      .setY(0)
      .setWidth(this->swapchain_extent_.width)
      .setHeight(this->swapchain_extent_.height)
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);

  vk::Rect2D scissor;
  scissor.setOffset({ 0, 0 }).setExtent(this->swapchain_extent_);

  vk::PipelineViewportStateCreateInfo viewport_state_ci;
  viewport_state_ci.setViewportCount(1).setPViewports(&viewport).setScissorCount(1).setPScissors(
      &scissor);

  vk::PipelineRasterizationStateCreateInfo rasterization_state_ci;
  rasterization_state_ci.setDepthClampEnable(VK_FALSE)
      .setRasterizerDiscardEnable(VK_FALSE)
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eClockwise)
      .setDepthBiasEnable(VK_FALSE);

  vk::PipelineMultisampleStateCreateInfo multisample_state_ci;
  multisample_state_ci.setSampleShadingEnable(VK_FALSE).setRasterizationSamples(
      vk::SampleCountFlagBits::e1);

  vk::PipelineColorBlendAttachmentState color_blend_attachment_state_ci;
  color_blend_attachment_state_ci.setColorWriteMask(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
  .setBlendEnable(VK_FALSE);

  this->device_.destroyShaderModule(frag_shader_module);
  this->device_.destroyShaderModule(vert_shader_module);
}

Application::Application()
{
  this->initSDL();
  this->initInstance();
  this->initPhysicalDevice();
  this->initQueueFamilies();
  this->initDevice();
  this->initSwapchain();
  this->initSwapchainImageViews();
  this->initGraphicsPipeline();
}

void Application::run()
{
  SDL_ShowWindow(this->window_);
  bool loop = true;
  while (loop)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_EventType::SDL_QUIT)
      {
        loop = false;
      }
    }
  }
  SDL_HideWindow(this->window_);
}

Application::~Application()
{
  // Destroy all image views
  for (auto& image_view : this->swapchain_image_views_)
    this->device_.destroyImageView(image_view);
  // Destroy the swapchain
  this->device_.destroySwapchainKHR(this->swapchain_);
  // Destroy the surface
  this->instance_.destroySurfaceKHR(this->surface_);
  // Destroy the logical device
  this->device_.destroy();
  // Destroy the instance
  this->instance_.destroy();
  // Destroy the window
  SDL_DestroyWindow(this->window_);
}
