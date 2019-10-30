#include "vulkan.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "submodules/tiny_obj_loader/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "submodules/stb-lib/stb_image.h"

const char* FRAG = "triangle.frag.spv";
const char* VERT = "triangle.vert.spv";
const char* TEXT = "chalet.jpg";
const char* OBJ  = "chalet.mdl";

#define VK_CHECK( value, info ) \
          if ( value != VK_SUCCESS ) { \
            printf( "ERROR: %s", info );\
            exit(EXIT_FAILURE); \
          }

void Vulkan::run(uint32_t width, uint32_t height, char* name ) {
  initGLFW( width, height, name );
  initVulkan();
  main();
  destroy();
}

void Vulkan::initVulkan() {
  // Shader loading
  //m_triangle  = Vertices::GetRectangle();
  m_rectangle = Vertices::GetRectangle();

  // Vulkan loading
  m_frameResized = false;
  createInstance();
  createSurface();
  m_physicalDevice = Utils::GetBestPhysicalDevice( m_instance );
  createDevice();
  createSwapchain();
  createImageView();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  createCommandPool();
  createDepthResources();
  createFrameBuffers();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();
  loadModel();
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();
}

void Vulkan::initGLFW(uint32_t width, uint32_t height, char* name ) {

  if ( !glfwInit() ) {
    printf( "An error occured when creating a window\n" );
    exit( EXIT_FAILURE );
  }

  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_W, false ) );
  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_S, false ) );
  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_A, false ) );
  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_D, false ) );
  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_UP, false ) );
  pressed.insert( std::pair< uint32_t, bool >( GLFW_KEY_DOWN, false ) );

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  m_window = glfwCreateWindow( width, height, name, nullptr, nullptr );
  glfwSetWindowUserPointer( m_window, this );
  glfwSetFramebufferSizeCallback( m_window, frameResizedCB );
  glfwSetKeyCallback( m_window, keyInputCB );
}

void Vulkan::main() {
  clock_t deltaTime = 0;
  unsigned int frames = 0;
  double  frameRate = 30;
  double  averageFrameTimeMilliseconds = 33.333;

  while ( !glfwWindowShouldClose( m_window ) ) {
    glfwPollEvents();

    clock_t start = clock();
    drawFrame();
    clock_t end = clock();

    deltaTime += end - start;
    frames++;

    if ( ( deltaTime / ( double ) ( CLOCKS_PER_SEC ) * 1000 ) > 1000) {
      frameRate = ( double ) frames * 0.5 + frameRate * 0.5;
      frames = 0;
      deltaTime -= CLOCKS_PER_SEC;
      averageFrameTimeMilliseconds = 1000.0 / ( frameRate == 0 ? 0.001 : frameRate );

      std::string title = "FPS: " + std::to_string( ( int )frameRate + 1 ) + " AVG FRAME TIME: " + std::to_string( averageFrameTimeMilliseconds );

      glfwSetWindowTitle( m_window, title.c_str() );
    }
  }

  vkDeviceWaitIdle( m_device );
}

void Vulkan::destroy() {
  invalidateSwapchain();

  vkDestroyImage( m_device, m_textureImage, nullptr );
  vkFreeMemory( m_device, m_textureImageMemory, nullptr );
  vkDestroyImageView( m_device, m_textureImageView, nullptr );
  vkDestroySampler( m_device, m_textureSampler, nullptr );

  vkDestroyDescriptorSetLayout( m_device, m_descriptorSetLayout, nullptr );

  vkDestroyBuffer( m_device, m_indexBuffer, nullptr );
  vkFreeMemory( m_device, m_indexMemory, nullptr );

  vkDestroyBuffer( m_device, m_vertexBuffer, nullptr );
  vkFreeMemory( m_device, m_vertexMemory, nullptr );

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore( m_device, m_renderFinishedSemaphores[i], nullptr );
    vkDestroySemaphore( m_device, m_imageAvailableSemaphores[i], nullptr );
    vkDestroyFence( m_device, m_inFlightFences[i], nullptr );
  }

  vkDestroyCommandPool( m_device, m_commandPool, nullptr );

  vkDestroyDevice( m_device, nullptr );
  vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
  vkDestroyInstance( m_instance, nullptr );

  glfwDestroyWindow( m_window );
  glfwTerminate();
}

void Vulkan::invalidateSwapchain() {
  vkDestroyImageView( m_device, m_depthImageView, nullptr );
  vkDestroyImage( m_device, m_depthImage, nullptr );
  vkFreeMemory( m_device, m_depthImageMemory, nullptr );

  for ( VkFramebuffer buffer : m_swapchainFramebuffers ) {
    vkDestroyFramebuffer( m_device, buffer, nullptr );
  }

  vkDestroyDescriptorPool( m_device, m_descriptorPool, nullptr );

  vkFreeCommandBuffers( m_device, m_commandPool, static_cast<uint32_t>( m_commandBuffers.size() ), m_commandBuffers.data() );
  vkDestroyPipeline( m_device, m_pipeline, nullptr );
  vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
  vkDestroyRenderPass( m_device, m_renderPass, nullptr );

  for ( VkImageView view : m_swapchainImageViews ) {
    vkDestroyImageView( m_device, view, nullptr );
  }

  for ( size_t i = 0; i < m_swapchainImages.size(); ++i ) {
    vkDestroyBuffer( m_device, m_uniformBuffers[i], nullptr );
    vkFreeMemory( m_device, m_uniformMemory[i], nullptr );
  }

  vkDestroySwapchainKHR( m_device, m_swapchain, nullptr );
}

void Vulkan::createInstance() {
  std::vector<const char*> extensions = getExtensions();

#ifdef __linux__
  extensions.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#elif _WIN32
  extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#endif

  std::vector< const char* > layers;
  //layers.push_back( "VK_LAYER_LUNARG_monitor" );
#ifndef NDEBUG
  layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
#endif

  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "VK_KOSTA";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "________";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo    = {};
  createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo        = &appInfo;
  createInfo.enabledExtensionCount   = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.ppEnabledLayerNames     = layers.data();
  createInfo.enabledLayerCount       = layers.size();

  VK_CHECK( vkCreateInstance( &createInfo, nullptr, &m_instance ), "Creating instance" );
}

std::vector<const char*> Vulkan::getExtensions() {
  uint32_t glfwExtensionCount;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

  std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

#ifndef NDEBUG
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif

  return extensions;
}

QueueFamilyIndices Vulkan::getGraphicQueue() {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount, nullptr );

  std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
  vkGetPhysicalDeviceQueueFamilyProperties( m_physicalDevice, &queueFamilyCount, queueFamilies.data() );

  uint32_t i = 0;
  for ( VkQueueFamilyProperties queueFamily : queueFamilies ) {
    if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR( m_physicalDevice, i, m_surface, &presentSupport );

    if (queueFamily.queueCount > 0 && presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

void Vulkan::createDevice() {
  float queuePriority = 1.0f;
  QueueFamilyIndices indices = getGraphicQueue();

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex        = indices.graphicsFamily;
  queueCreateInfo.queueCount              = 1;
  queueCreateInfo.pQueuePriorities        = &queuePriority;

  queueCreateInfos.push_back(queueCreateInfo);


  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy        = VK_TRUE;

  std::vector<const char*> extensions;
  extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

  VkDeviceCreateInfo createInfo      = {};
  createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos       = &queueCreateInfo;
  createInfo.queueCreateInfoCount    = 1;
  createInfo.pEnabledFeatures        = &deviceFeatures;
  createInfo.enabledExtensionCount   = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();

  VK_CHECK( vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_device ), "Creating device" );

  vkGetDeviceQueue( m_device, indices.graphicsFamily, 0, &m_graphicsQueue );
  vkGetDeviceQueue( m_device, indices.presentFamily, 0, &m_presentQueue );
}

void Vulkan::createSurface() {
  VK_CHECK( glfwCreateWindowSurface( m_instance, m_window, nullptr, &m_surface ), "Creating surface" );
}

SwapChainSupportDetails Vulkan::getSwapchainSupport() {
  SwapChainSupportDetails details;

  // Format
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR( m_physicalDevice, m_surface, &formatCount, nullptr );

  if ( formatCount != 0 ) {
    details.formats.resize( formatCount );
    vkGetPhysicalDeviceSurfaceFormatsKHR( m_physicalDevice, m_surface, &formatCount, details.formats.data() );
  }

  // Present
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR( m_physicalDevice, m_surface, &presentModeCount, nullptr );

  if ( presentModeCount != 0 ) {
    details.presentModes.resize( presentModeCount );
    vkGetPhysicalDeviceSurfacePresentModesKHR( m_physicalDevice, m_surface, &presentModeCount, details.presentModes.data() );
  }

  // Capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_physicalDevice, m_surface, &details.capabilities );

  return details;
}

VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats ) {
  for ( const auto& availableFormat : availableFormats ) {
    if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes ) {
  for ( const auto& availablePresentMode : availablePresentModes ) {
    if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities ) {
  if ( capabilities.currentExtent.width != UINT32_MAX ) {
    return capabilities.currentExtent;
  } else {
    int32_t width, height;
    glfwGetFramebufferSize( m_window, &width, &height );

    VkExtent2D actualExtent = {( uint32_t )width, ( uint32_t )height};

    actualExtent.width  = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
    actualExtent.height = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

    return actualExtent;
  }
}

void Vulkan::createSwapchain() {
  SwapChainSupportDetails swapChainSupport = getSwapchainSupport();
  VkSurfaceFormatKHR surfaceFormat         = chooseSwapSurfaceFormat( swapChainSupport.formats );
  VkPresentModeKHR presentMode             = chooseSwapPresentMode( swapChainSupport.presentModes );
  VkExtent2D extent                        = chooseSwapExtent( swapChainSupport.capabilities );

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface                  = m_surface;
  createInfo.imageFormat              = surfaceFormat.format;
  createInfo.imageColorSpace          = surfaceFormat.colorSpace;
  createInfo.minImageCount            = swapChainSupport.capabilities.minImageCount;
  createInfo.imageExtent              = swapChainSupport.capabilities.currentExtent;
  createInfo.presentMode              = presentMode;
  createInfo.oldSwapchain             = VK_NULL_HANDLE;
  createInfo.preTransform             = swapChainSupport.capabilities.currentTransform;
  createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount    = 0;
  createInfo.pQueueFamilyIndices      = nullptr;
  createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageArrayLayers         = swapChainSupport.capabilities.maxImageArrayLayers;
  createInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.clipped                  = VK_TRUE;

  VK_CHECK( vkCreateSwapchainKHR( m_device, &createInfo, nullptr, &m_swapchain ), "Creating swapchain" );

  vkGetSwapchainImagesKHR( m_device, m_swapchain, &swapChainSupport.capabilities.minImageCount, nullptr );
  m_swapchainImages.resize( swapChainSupport.capabilities.minImageCount );
  vkGetSwapchainImagesKHR( m_device, m_swapchain, &swapChainSupport.capabilities.minImageCount, m_swapchainImages.data() );

  m_swapchainImageFormat = surfaceFormat.format;
  m_swapchainExtent      = extent;
}

void Vulkan::createImageView() {
  m_swapchainImageViews.resize( m_swapchainImages.size() );

  for ( uint32_t i = 0; i < m_swapchainImages.size(); ++i ) {
    m_swapchainImageViews[i] = createImageView( m_swapchainImages[i], m_swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT );
  }
}

void Vulkan::createRenderPass() {
  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = 0;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format                  = findDepthFormat();
  depthAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format                  = m_swapchainImageFormat;
  colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment            = 1;
  depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment            = 0;
  colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass    = {};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  std::vector< VkAttachmentDescription > attachments = { colorAttachment, depthAttachment };

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount        = attachments.size();
  renderPassInfo.pAttachments           = attachments.data();
  renderPassInfo.subpassCount           = 1;
  renderPassInfo.pSubpasses             = &subpass;
  renderPassInfo.pDependencies          = &dependency;
  renderPassInfo.dependencyCount        = 1;

  VK_CHECK( vkCreateRenderPass( m_device, &renderPassInfo, nullptr, &m_renderPass ), "Creating renderpass" );
}

void Vulkan::createGraphicsPipeline() {
  VkVertexInputBindingDescription bindingDescription                     = Vertex::getBindingDescription();
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();

  VkShaderModule vertTriangle = nullptr;
  VkShaderModule fragTriangle = nullptr;
  createShaderModule( Utils::readFile( VERT ), &vertTriangle );
  createShaderModule( Utils::readFile( FRAG ), &fragTriangle );

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module                          = vertTriangle;
  vertShaderStageInfo.pName                           = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module                          = fragTriangle;
  fragShaderStageInfo.pName                           = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount        = 1;
  vertexInputInfo.pVertexBindingDescriptions           = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount      = attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions         = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable                 = VK_FALSE;

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = (float) m_swapchainExtent.width;
  viewport.height     = (float) m_swapchainExtent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent   = m_swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount                     = 1;
  viewportState.pViewports                        = &viewport;
  viewportState.scissorCount                      = 1;
  viewportState.pScissors                         = &scissor;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable                       = VK_TRUE;
  depthStencil.depthWriteEnable                      = VK_TRUE;
  depthStencil.depthCompareOp                        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable                 = VK_FALSE;
  depthStencil.minDepthBounds                        = 0.0f;
  depthStencil.maxDepthBounds                        = 1.0f;
  depthStencil.stencilTestEnable                     = VK_FALSE;
  depthStencil.front                                 = {};
  depthStencil.back                                  = {};

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable                       = VK_FALSE;
  rasterizer.rasterizerDiscardEnable                = VK_FALSE;
  rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth                              = 1.0f;
  rasterizer.cullMode                               = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable                        = VK_FALSE;
  rasterizer.depthBiasConstantFactor                = 0.0f;
  rasterizer.depthBiasClamp                         = 0.0f;
  rasterizer.depthBiasSlopeFactor                   = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable                  = VK_FALSE;
  multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading                     = 1.0f;
  multisampling.pSampleMask                          = nullptr;
  multisampling.alphaToCoverageEnable                = VK_FALSE;
  multisampling.alphaToOneEnable                     = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable                         = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp                        = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp                        = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable                       = VK_FALSE;
  colorBlending.logicOp                             = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount                     = 1;
  colorBlending.pAttachments                        = &colorBlendAttachment;
  colorBlending.blendConstants[0]                   = 0.0f;
  colorBlending.blendConstants[1]                   = 0.0f;
  colorBlending.blendConstants[2]                   = 0.0f;
  colorBlending.blendConstants[3]                   = 0.0f;

  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount                = 2;
  dynamicState.pDynamicStates                   = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount             = 1;
  pipelineLayoutInfo.pSetLayouts                = &m_descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount     = 0;
  pipelineLayoutInfo.pPushConstantRanges        = nullptr;

  VK_CHECK( vkCreatePipelineLayout( m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout ), "Creating pipeline layout" );

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount                   = 2;
  pipelineInfo.pStages                      = shaderStages;
  pipelineInfo.pVertexInputState            = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState          = &inputAssembly;
  pipelineInfo.pViewportState               = &viewportState;
  pipelineInfo.pRasterizationState          = &rasterizer;
  pipelineInfo.pMultisampleState            = &multisampling;
  pipelineInfo.pColorBlendState             = &colorBlending;
  pipelineInfo.pDepthStencilState           = &depthStencil;
  pipelineInfo.layout                       = m_pipelineLayout;
  pipelineInfo.renderPass                   = m_renderPass;
  pipelineInfo.subpass                      = 0;
  pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;

  VK_CHECK(vkCreateGraphicsPipelines( m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline ), "Creating graphics pipeline" );

  vkDestroyShaderModule( m_device, vertTriangle, nullptr );
  vkDestroyShaderModule( m_device, fragTriangle, nullptr );
}

void Vulkan::createShaderModule(std::vector< char > code, VkShaderModule* shaderModule ) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize                 = code.size();
  createInfo.pCode                    = ( uint32_t* )code.data();

  VK_CHECK( vkCreateShaderModule( m_device, &createInfo, nullptr, shaderModule ), "Creating shader module" );
}

void Vulkan::createFrameBuffers() {
  m_swapchainFramebuffers.resize( m_swapchainImageViews.size() );

  for ( size_t i = 0; i < m_swapchainImageViews.size(); ++i ) {
    std::vector< VkImageView > attachments = { m_swapchainImageViews[i], m_depthImageView };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass              = m_renderPass;
    framebufferInfo.attachmentCount         = attachments.size();
    framebufferInfo.pAttachments            = attachments.data();
    framebufferInfo.width                   = m_swapchainExtent.width;
    framebufferInfo.height                  = m_swapchainExtent.height;
    framebufferInfo.layers                  = 1;

    VK_CHECK( vkCreateFramebuffer( m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i] ), "Creating framebuffer" );
  }
}

void Vulkan::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex        = getGraphicQueue().graphicsFamily;
  poolInfo.flags                   = 0;

  VK_CHECK( vkCreateCommandPool( m_device, &poolInfo, nullptr, &m_commandPool ), "Creating command pool" );
}

void Vulkan::createCommandBuffers() {
  m_commandBuffers.resize( m_swapchainFramebuffers.size() );

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount          = (uint32_t) m_commandBuffers.size();

  VK_CHECK( vkAllocateCommandBuffers( m_device, &allocInfo, m_commandBuffers.data() ), "Allocating command buffers" );

  for (size_t i = 0; i < m_commandBuffers.size(); ++i) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = 0;
    beginInfo.pInheritanceInfo         = nullptr;

    VK_CHECK( vkBeginCommandBuffer( m_commandBuffers[i], &beginInfo ), "Beginning command buffer" );

    std::vector< VkClearValue > clearValues ( 2 );
    clearValues[0].color        = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass            = m_renderPass;
    renderPassInfo.framebuffer           = m_swapchainFramebuffers[i];
    renderPassInfo.renderArea.offset.x   = 0;
    renderPassInfo.renderArea.offset.y   = 0;
    renderPassInfo.renderArea.extent     = m_swapchainExtent;
    renderPassInfo.clearValueCount       = clearValues.size();
    renderPassInfo.pClearValues          = clearValues.data();

    vkCmdBeginRenderPass( m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

    vkCmdBindPipeline( m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline );

    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers( m_commandBuffers[i], 0, 1, vertexBuffers, offsets );
    vkCmdBindIndexBuffer( m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32 );
    vkCmdBindDescriptorSets( m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr );

    vkCmdDrawIndexed( m_commandBuffers[i], m_rectIndices.size(), 1, 0, 0, 0 );

    vkCmdEndRenderPass( m_commandBuffers[i] );

    VK_CHECK( vkEndCommandBuffer( m_commandBuffers[i] ), "Ending command buffer");
  }
}

void Vulkan::createSyncObjects() {
  m_imageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
  m_renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
  m_inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      VK_CHECK( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i] ), "Creating semaphore" );
      VK_CHECK( vkCreateSemaphore( m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i] ), "Creating semaphore" );
      VK_CHECK( vkCreateFence( m_device, &fenceInfo, nullptr, &m_inFlightFences[i] ), "Creating fence" );
  }
}

void Vulkan::drawFrame() {
  VK_CHECK( vkWaitForFences( m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX ), "Waiting fence" );
  if( m_frameResized ) {
    updateSwapchain();
  }

  uint32_t imageIndex;
  vkAcquireNextImageKHR( m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex );

  VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSemaphore signalSemaphores[]  = {m_renderFinishedSemaphores[m_currentFrame]};

  updateUniformBuffer( imageIndex );

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = waitSemaphores;
  submitInfo.pWaitDstStageMask  = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &m_commandBuffers[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  VK_CHECK( vkResetFences( m_device, 1, &m_inFlightFences[m_currentFrame] ), "Reseting fence" );
  VkResult status = vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame] );

  switch( status ) {
    case VK_ERROR_OUT_OF_DATE_KHR:
      updateSwapchain();
      return;
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    default:
      VK_CHECK( status, "Submiting queue" );
  }

  VkPresentInfoKHR presentInfo   = {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  VkSwapchainKHR swapChains[] = {m_swapchain};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapChains;
  presentInfo.pImageIndices   = &imageIndex;
  presentInfo.pResults        = nullptr;

  status = vkQueuePresentKHR( m_presentQueue, &presentInfo );
  switch( status ) {
    case VK_ERROR_OUT_OF_DATE_KHR:
      updateSwapchain();
      return;
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    default:
      VK_CHECK( status, "Presenting" );
  }

  vkQueueWaitIdle( m_presentQueue );

  m_currentFrame = ( m_currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::updateSwapchain() {
  int width = 0, height = 0;
  while ( width == 0 || height == 0 ) {
    glfwGetFramebufferSize( m_window, &width, &height );
    glfwWaitEvents();
  }

  m_frameResized = false;
  vkDeviceWaitIdle( m_device );

  invalidateSwapchain();

  createSwapchain();
  createImageView();
  createRenderPass();
  createGraphicsPipeline();
  createDepthResources();
  createFrameBuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
}

void Vulkan::frameResizedCB(GLFWwindow* window, int width, int height ) {
  auto app = reinterpret_cast<Vulkan*>( glfwGetWindowUserPointer(window ) );
  app->m_frameResized = true;
}

void Vulkan::createVertexBuffer() {
  VkDeviceSize bufferSize = m_triangle.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

  void* pData;
  VK_CHECK( vkMapMemory( m_device, stagingBufferMemory, 0, bufferSize, 0, &pData ), "Mapping vertex memory" );
  memcpy( pData, m_triangle.shader.data(), ( size_t )bufferSize );
  vkUnmapMemory( m_device, stagingBufferMemory );

  createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexMemory );
  copyBuffer( stagingBuffer, m_vertexBuffer, bufferSize );
  vkDestroyBuffer( m_device, stagingBuffer, nullptr );
  vkFreeMemory( m_device, stagingBufferMemory, nullptr );
}

void Vulkan::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) {
  // Should replace by a predefined commandPool and commandBuffer
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {};
  copyRegion.size         = size;

  vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

  endSingleTimeCommands( commandBuffer );
}

void Vulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory ) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = size;
  bufferInfo.usage              = usage;
  bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK( vkCreateBuffer( m_device, &bufferInfo, nullptr, &buffer ), "Creating buffer" );

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements( m_device, buffer, &memRequirements );

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = findMemoryType( memRequirements.memoryTypeBits, properties );

  VK_CHECK( vkAllocateMemory( m_device, &allocInfo, nullptr, &bufferMemory ), "Allocating memory" );
  VK_CHECK( vkBindBufferMemory( m_device, buffer, bufferMemory, 0 ), "Binding memory" );
}

void Vulkan::createIndexBuffer() {
  VkDeviceSize bufferSize = sizeof( m_rectIndices[0] ) * m_rectIndices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory );

  void* data;
  vkMapMemory( m_device, stagingBufferMemory, 0, bufferSize, 0, &data );
  memcpy( data, m_rectIndices.data(), (size_t) bufferSize );
  vkUnmapMemory( m_device, stagingBufferMemory );

  createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexMemory );

  copyBuffer( stagingBuffer, m_indexBuffer, bufferSize );

  vkDestroyBuffer( m_device, stagingBuffer, nullptr );
  vkFreeMemory( m_device, stagingBufferMemory, nullptr );
}

void Vulkan::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding                      = 0;
  uboLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount              = 1;
  uboLayoutBinding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers           = nullptr;

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding                      = 1;
  samplerLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount              = 1;
  samplerLayoutBinding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers           = nullptr;

  std::vector<VkDescriptorSetLayoutBinding> layouts = { uboLayoutBinding, samplerLayoutBinding };

  VkDescriptorSetLayoutCreateInfo createInfo = {};
  createInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  createInfo.bindingCount                    = layouts.size();
  createInfo.pBindings                       = layouts.data();

  VK_CHECK( vkCreateDescriptorSetLayout( m_device, &createInfo, nullptr, &m_descriptorSetLayout ), "Creating layout descriptor" );
}

void Vulkan::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof( UniformBufferObject );

  m_uniformBuffers.resize( m_swapchainImages.size() );
  m_uniformMemory.resize( m_swapchainImages.size() );

  for ( size_t i = 0; i < m_swapchainImages.size(); ++i ) {
    createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformMemory[i] );
  }
}

void Vulkan::updateUniformBuffer(uint32_t currentImage ) {
  updateCamera();

  UniformBufferObject ubo = {};
  ubo.model               = glm::translate(
          glm::rotate( glm::mat4( 1 ), glm::radians( 90.0f ), glm::vec3( 1, 0, 0 ) ), glm::vec3( 0, 0, -1 ) );
  ubo.view                = glm::lookAt( eye, look_at, up );
  ubo.proj                = glm::perspective<float>(glm::radians(90.0f), m_swapchainExtent.width / (float) m_swapchainExtent.height, 0.1f, 100.0f);
  ubo.proj[1][1]          *= -1;

  void* data;
  vkMapMemory( m_device, m_uniformMemory[currentImage], 0, sizeof( ubo ), 0, &data );
  memcpy( data, &ubo, sizeof( ubo ) );
  vkUnmapMemory( m_device, m_uniformMemory[currentImage] );
}

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties ) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties( m_physicalDevice, &memProperties );

  for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i ) {
    if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties ) {
      return i;
    }
  }

  return UINT32_MAX;
}

void Vulkan::createDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSize( 2 );
  poolSize[0]                 = {};
  poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize[0].descriptorCount = m_swapchainImages.size();
  poolSize[1]                 = {};
  poolSize[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize[1].descriptorCount = m_swapchainImages.size();

  VkDescriptorPoolCreateInfo createInfo = {};
  createInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.poolSizeCount              = poolSize.size();
  createInfo.pPoolSizes                 = poolSize.data();
  createInfo.maxSets                    = m_swapchainImages.size();

  VK_CHECK( vkCreateDescriptorPool( m_device, &createInfo, nullptr, &m_descriptorPool ),"Creating descriptor pool" );
}

void Vulkan::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts( m_swapchainImages.size(), m_descriptorSetLayout );
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool              = m_descriptorPool;
  allocInfo.descriptorSetCount          = m_swapchainImages.size();
  allocInfo.pSetLayouts                 = layouts.data();

  m_descriptorSets.resize( m_swapchainImages.size() );
  VK_CHECK( vkAllocateDescriptorSets( m_device, &allocInfo, m_descriptorSets.data() ), "Allocating descriptor sets" );

  for ( size_t i = 0; i < m_swapchainImages.size(); ++i ) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer                 = m_uniformBuffers[i];
    bufferInfo.offset                 = 0;
    bufferInfo.range                  = sizeof( UniformBufferObject );

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView             = m_textureImageView;
    imageInfo.sampler               = m_textureSampler;

    std::vector<VkWriteDescriptorSet> descriptorWrite ( 2 );
    descriptorWrite[0]                      = {};
    descriptorWrite[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet               = m_descriptorSets[i];
    descriptorWrite[0].dstBinding           = 0;
    descriptorWrite[0].dstArrayElement      = 0;
    descriptorWrite[0].descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[0].descriptorCount      = 1;
    descriptorWrite[0].pBufferInfo          = &bufferInfo;

    descriptorWrite[1]                      = {};
    descriptorWrite[1].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].dstSet               = m_descriptorSets[i];
    descriptorWrite[1].dstBinding           = 1;
    descriptorWrite[1].dstArrayElement      = 0;
    descriptorWrite[1].descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount      = 1;
    descriptorWrite[1].pImageInfo           = &imageInfo;

    vkUpdateDescriptorSets( m_device, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr );
  }
}

void Vulkan::createTextureImage () {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels        = stbi_load( TEXT, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  if (!pixels) {
    printf( "ERROR: Loading texture" );
    exit( EXIT_FAILURE );
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory );

  void* data;
  vkMapMemory( m_device, stagingMemory, 0, imageSize, 0, &data );
  memcpy( data, pixels, imageSize );
  vkUnmapMemory( m_device, stagingMemory );

  stbi_image_free( pixels );

  createImage(
      texWidth,
      texHeight,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_textureImage,
      m_textureImageMemory
      );

  transitionImageLayout( m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
  copyBufferToImage( stagingBuffer, m_textureImage, texWidth, texHeight );
  transitionImageLayout( m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

  vkDestroyBuffer( m_device, stagingBuffer, nullptr );
  vkFreeMemory( m_device, stagingMemory, nullptr );
}

void Vulkan::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory ) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = width;
  imageInfo.extent.height     = height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = 1;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = format;
  imageInfo.tiling            = tiling;
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = usage;
  imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

  VK_CHECK( vkCreateImage( m_device, &imageInfo, nullptr, &image ), "Creating image" );

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements( m_device, image, &memReqs );

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memReqs.size;
  allocInfo.memoryTypeIndex      = findMemoryType( memReqs.memoryTypeBits, properties );

  VK_CHECK( vkAllocateMemory( m_device, &allocInfo, nullptr, &imageMemory ), "Allocating image memory" );
  VK_CHECK( vkBindImageMemory( m_device, image, imageMemory, 0 ), "Binding image memory" );
}

VkCommandBuffer Vulkan::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.commandBufferCount          = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers( m_device, &allocInfo, &commandBuffer );

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer( commandBuffer, &beginInfo );

  return commandBuffer;
}

void Vulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer ) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( m_graphicsQueue );

  vkFreeCommandBuffers( m_device, m_commandPool, 1, &commandBuffer );
}

void Vulkan::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout ) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.srcAccessMask                   = 0;
  barrier.dstAccessMask                   = 0;

  if ( newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if ( hasStencilComponent( format ) ) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    printf( "ERROR: Finding mask for image transition" );
    exit( EXIT_FAILURE );
  }

  vkCmdPipelineBarrier(
      commandBuffer,
      sourceStage,
      destinationStage,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier
  );

  endSingleTimeCommands(commandBuffer);
}

void Vulkan::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height ) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;
  region.imageOffset       = {0, 0, 0};
  region.imageExtent       = { width, height, 1 };

  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;

  vkCmdCopyBufferToImage(
      commandBuffer,
      buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region
  );

  endSingleTimeCommands( commandBuffer );
}

void Vulkan::createTextureImageView() {
  m_textureImageView = createImageView( m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT );
}

VkImageView Vulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags ) {
  VkImageViewCreateInfo viewInfo           = {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkImageView imageView;
  VK_CHECK( vkCreateImageView( m_device, &viewInfo, nullptr, &imageView ), "Creating texture image view" );

  return imageView;
}

void Vulkan::createTextureSampler() {
  VkSamplerCreateInfo samplerInfo     = {};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 8;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = 0.0f;

  VK_CHECK( vkCreateSampler( m_device, &samplerInfo, nullptr, &m_textureSampler ), "Creating texture sampler" );
}

void Vulkan::createDepthResources() {
  VkFormat depthFormat = findDepthFormat();

  createImage(
      m_swapchainExtent.width,
      m_swapchainExtent.height,
      depthFormat,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_depthImage,
      m_depthImageMemory
      );

  m_depthImageView = createImageView( m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT );
  transitionImageLayout( m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

VkFormat Vulkan::findDepthFormat() {
  return findSupportedFormat(
      { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

bool Vulkan::hasStencilComponent(VkFormat format ) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat Vulkan::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features ) {
  for ( VkFormat format : candidates ) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( m_physicalDevice, format, &props );

    if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features ) {
      return format;
    } else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features ) {
      return format;
    }
  }

  printf( "ERROR: No supported image tilting format\n");
  exit( EXIT_FAILURE );
}

void Vulkan::loadModel() {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if ( !tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, OBJ ) ) {
    printf( "ERROR: %s", err.c_str() );
    exit( EXIT_FAILURE );
  }

  std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

  for ( tinyobj::shape_t& shape : shapes ) {
    for ( tinyobj::index_t& index : shape.mesh.indices ) {
      Vertex vertex = {};

      vertex.pos = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
      };

      vertex.texCoord = {
          attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
      };

      vertex.color = {1.0f, 1.0f, 1.0f};

      if ( uniqueVertices.count(vertex) == 0 ) {
        uniqueVertices[vertex] = static_cast<uint32_t>( m_triangle.shader.size() );
        m_triangle.shader.push_back(vertex);
      }

      m_rectIndices.push_back( uniqueVertices[vertex] );
    }
  }
}

void Vulkan::keyInputCB(GLFWwindow* window, int key, int scancode, int action, int mods ) {

  auto app = reinterpret_cast<Vulkan*>( glfwGetWindowUserPointer(window ) );
  float value = 0.3;
  glm::vec3 inc = { 0.0f, 0.0f, 0.0f };

  if( action != GLFW_REPEAT ) {
    if( key == GLFW_KEY_W ) {
      app->pressed.at( GLFW_KEY_W ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_S ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_S ) = false;
      }
    } else if ( key == GLFW_KEY_S ) {
      app->pressed.at( GLFW_KEY_S ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_W ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_W ) = false;
      }
    } else if ( key == GLFW_KEY_A ) {
      app->pressed.at( GLFW_KEY_A ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_D ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_D ) = false;
      }
    } else if ( key == GLFW_KEY_D ) {
      app->pressed.at( GLFW_KEY_D ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_A ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_A ) = false;
      }
    } else if ( key == GLFW_KEY_UP ) {
      app->pressed.at( GLFW_KEY_UP ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_DOWN ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_DOWN ) = false;
      }
    } else if ( key == GLFW_KEY_DOWN ) {
      app->pressed.at( GLFW_KEY_DOWN ) = ( action == GLFW_PRESS );
      if( app->pressed.at( GLFW_KEY_UP ) && action == GLFW_PRESS ) {
        app->pressed.at( GLFW_KEY_UP ) = false;
      }
    }
  }

  if( app->pressed.at( GLFW_KEY_W ) ) {
    inc.y = value;
  } else if ( app->pressed.at( GLFW_KEY_S ) ) {
    inc.y = -value;
  }

  if( app->pressed.at( GLFW_KEY_A ) ) {
    inc.x = value;
  } else if ( app->pressed.at( GLFW_KEY_D ) ) {
    inc.x = -value;
  }

  if( app->pressed.at( GLFW_KEY_UP ) ) {
    inc.z = value;
  } else if ( app->pressed.at( GLFW_KEY_DOWN ) ) {
    inc.z = -value;
  }

  app->smoothCameraMovement( inc );
}

void Vulkan::smoothCameraMovement(glm::vec3 inc ) {
  m_smoothCamera += inc;
}

void Vulkan::updateCamera() {
  float step = 0.05;

  if( abs( m_smoothCamera.x ) >= step ) {
    if ( m_smoothCamera.x > 0 ) {
      m_smoothCamera.x -= step;
      eye.x += step;
    } else {
      m_smoothCamera.x += step;
      eye.x -= step;
    }
  } else if ( abs( m_smoothCamera.x ) < step ) {
    eye.x += m_smoothCamera.x;
    m_smoothCamera.x = 0;
  }

  if( abs( m_smoothCamera.y ) >= step ) {
    if ( m_smoothCamera.y > 0 ) {
      m_smoothCamera.y -= step;
      eye.y += step;
    } else {
      m_smoothCamera.y += step;
      eye.y -= step;
    }
  } else if ( abs( m_smoothCamera.y ) < step ) {
    eye.y += m_smoothCamera.y;
    m_smoothCamera.y = 0;
  }

  if( abs( m_smoothCamera.z ) >= step ) {
    if ( m_smoothCamera.z > 0 ) {
      m_smoothCamera.z -= step;
      eye.z += step;
    } else {
      m_smoothCamera.z += step;
      eye.z -= step;
    }
  } else if ( abs( m_smoothCamera.z ) < step ) {
    eye.z += m_smoothCamera.z;
    m_smoothCamera.z = 0;
  }
}


