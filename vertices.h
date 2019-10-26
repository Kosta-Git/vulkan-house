//
// Created by Kosta Sovaridis on 9/3/19.
//

#ifndef VULKAN_VERTICES_H
#define VULKAN_VERTICES_H

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_HAS_CXX11_STL 1
#include <glm/glm.hpp>

struct Vertex {
public:
  alignas(16) glm::vec3 pos{};
  alignas(16) glm::vec3 color{};
  alignas(16) glm::vec2 texCoord{};
  /*
  Vertex( glm::vec3 position, glm::vec3 colors, glm::vec2 texCoords ) {
    pos      = position;
    color    = colors;
    texCoord = texCoords;
  }
   */

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                         = 0;
    bindingDescription.stride                          = sizeof( Vertex );
    bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
    attributeDescriptions[0].binding                                       = 0;
    attributeDescriptions[0].location                                      = 0;
    attributeDescriptions[0].format                                        = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset                                        = offsetof(Vertex, pos);
    attributeDescriptions[1].binding                                       = 0;
    attributeDescriptions[1].location                                      = 1;
    attributeDescriptions[1].format                                        = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset                                        = offsetof(Vertex, color);
    attributeDescriptions[2].binding                                       = 0;
    attributeDescriptions[2].location                                      = 2;
    attributeDescriptions[2].format                                        = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset                                        = offsetof(Vertex, texCoord);
    return attributeDescriptions;
  }

  bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
  }
};

struct Shader {
public:
  std::vector<Vertex> shader;

  uint32_t size() {
    return sizeof( shader[0] ) * shader.size();
  }
};

namespace Vertices {
  Shader GetTriangle();
  Shader GetRectangle();
  Shader GetPent();
};


#endif //VULKAN_VERTICES_H
