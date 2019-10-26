#ifndef VULKAN_TRIANGLE_H
#define VULKAN_TRIANGLE_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_macos.h>

#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#elif __linux__
#define GLFW_EXPOSE_NATIVE_X11
# elif _WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIAN
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_HAS_CXX11_STL 1
#include <glm/gtx/hash.hpp>

#include <cmath>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <map>

#include "utils.h"
#include "vertices.h"

namespace std {
  template<> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
      return ((hash<glm::vec3>()(vertex.pos) ^
               (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
             (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   presentModes;
};

struct QueueFamilyIndices {
  uint32_t graphicsFamily;
  uint32_t presentFamily;

  bool isComplete() {
    return graphicsFamily && presentFamily;
  }
};

struct UniformBufferObject {
  alignas( 16 ) glm::mat4 model;
  alignas( 16 ) glm::mat4 view;
  alignas( 16 ) glm::mat4 proj;
};

class Vulkan {
public:
  void run( uint32_t width, uint32_t height, char* name );
  bool m_frameResized;

  glm::vec3 eye     = glm::vec3(1.0f, 0.0f, 2.0f);
  glm::vec3 look_at = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 up      = glm::vec3(0.0f, -1.0f, 0.0f);
  std::map< uint32_t, bool > pressed;
  void smoothCameraMovement( glm::vec3 inc );

private:
  void initVulkan();
  void initGLFW( uint32_t width, uint32_t height, char* name );
  void main();
  void destroy();
  void drawFrame();
  void createInstance();
  void createDevice();
  void createSurface();
  void createSwapchain();
  void createImageView();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFrameBuffers();
  void createCommandPool();
  void createVertexBuffer();
  void createCommandBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createSyncObjects();
  void createIndexBuffer();
  void createDescriptorSetLayout();
  void updateUniformBuffer( uint32_t currentImage );
  void createUniformBuffers();
  void createTextureImage();
  void createTextureImageView();
  void createTextureSampler();
  void createDepthResources();
  void loadModel();
  void createBuffer( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory );
  void copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
  void invalidateSwapchain();
  void updateSwapchain();
  static void frameResizedCB( GLFWwindow* window, int width, int height );
  static void keyInputCB( GLFWwindow* window, int key, int scancode, int action, int mods );
  void createShaderModule( std::vector< char > code, VkShaderModule* shaderModule );
  std::vector<const char*> getExtensions();
  QueueFamilyIndices getGraphicQueue();
  SwapChainSupportDetails getSwapchainSupport();
  VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );
  VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );
  VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
  uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );
  void createImage( uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory );
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands( VkCommandBuffer commandBuffer );
  void transitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
  void copyBufferToImage( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );
  VkImageView createImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );
  VkFormat findSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features );
  VkFormat findDepthFormat();
  bool hasStencilComponent( VkFormat format );
  void updateCamera();

  uint32_t width;
  uint32_t height;

  GLFWwindow* m_window;
  VkInstance m_instance;
  VkPhysicalDevice m_physicalDevice;
  VkDevice m_device;
  VkSurfaceKHR m_surface;
  VkSwapchainKHR m_swapchain;
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  VkFormat m_swapchainImageFormat;
  VkExtent2D m_swapchainExtent;
  VkRenderPass m_renderPass;
  std::vector<VkFramebuffer> m_swapchainFramebuffers;
  VkPipelineLayout m_pipelineLayout;
  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers;
  VkPipeline m_pipeline;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence> m_inFlightFences;
  VkQueue m_presentQueue;
  VkQueue m_graphicsQueue;
  VkBuffer m_vertexBuffer;
  VkDeviceMemory m_vertexMemory;
  VkBuffer m_indexBuffer;
  VkDeviceMemory m_indexMemory;
  VkDescriptorSetLayout m_descriptorSetLayout;
  std::vector<VkBuffer> m_uniformBuffers;
  std::vector<VkDeviceMemory> m_uniformMemory;
  VkDescriptorPool m_descriptorPool;
  std::vector<VkDescriptorSet> m_descriptorSets;
  VkImage m_textureImage;
  VkDeviceMemory m_textureImageMemory;
  VkImageView m_textureImageView;
  VkSampler m_textureSampler;
  VkImage m_depthImage;
  VkDeviceMemory m_depthImageMemory;
  VkImageView m_depthImageView;

  const int MAX_FRAMES_IN_FLIGHT = 2;
  size_t    m_currentFrame       = 0;

  glm::vec3 m_smoothCamera = { 0, 0, 0 };

  Shader m_triangle;
  Shader m_rectangle;
  std::vector<uint32_t> m_rectIndices;// = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };
};

#endif
