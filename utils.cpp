//
// Created by Kosta Sovaridis on 9/2/19.
//

#include "utils.h"

VkPhysicalDevice Utils::GetBestPhysicalDevice( VkInstance instance ) {
  uint32_t deviceCount;
  std::vector<VkPhysicalDevice> devices;
  std::vector<uint32_t> devicePoints;
  vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );
  devices.resize( deviceCount );
  devicePoints.resize( deviceCount );
  vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

  VkPhysicalDeviceProperties props;
  for( uint32_t i = 0; i < deviceCount; ++i ) {
    vkGetPhysicalDeviceProperties( devices[i], &props );

    switch ( props.deviceType ) {
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        //return devices[ i ];
        devicePoints[i] = 1000;
        break;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        devicePoints[i] = 500;
        break;
      default:
        devicePoints[i] = 0;
        break;
    }

    devicePoints[i] += props.limits.maxImageDimension2D;
  }

  uint32_t maxIndex = 0;
  uint32_t maxValue = 0;
  for( uint32_t i = 0; i < deviceCount; ++i ) {
    if( maxValue < devicePoints[i] ) {
      maxIndex = i;
      maxValue = devicePoints[i];
    }
  }

  return devices[maxIndex];
}

std::vector< char > Utils::readFile( std::string filename ) {
  std::ifstream file( filename, std::ios::ate | std::ios::binary );

  if (!file.is_open()) {
    throw std::runtime_error("Failed to locate shader...");
  }

  auto fileSize = file.tellg();
  std::vector< char > buffer( ( long long int )fileSize );
  file.seekg( 0 );
  file.read( buffer.data(), fileSize );
  file.close();

  return buffer;
}