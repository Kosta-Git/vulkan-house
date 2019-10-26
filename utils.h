//
// Created by Kosta Sovaridis on 9/2/19.
//

#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include <vulkan/vulkan.h>
#include <cstdlib>
#include <printf.h>
#include <vector>
#include <string>
#include <fstream>

namespace Utils {
  VkPhysicalDevice GetBestPhysicalDevice( VkInstance instance );
  std::vector< char > readFile( std::string filename );
}

#endif //VULKAN_UTILS_H
