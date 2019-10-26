#include "vulkan.h"

int main() {
  printf( "Starting app...\n");

  Vulkan app;
  app.run( 1000, 800, "VK" );

  printf("Stopping app...\n");
  return EXIT_SUCCESS;
}