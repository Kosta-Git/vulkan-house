//
// Created by Kosta Sovaridis on 9/3/19.
//

#include "vertices.h"

Shader Vertices::GetTriangle() {
  Shader triangle;
  triangle.shader = {
      { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
      { { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
      { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
  };

  return triangle;
}

Shader Vertices::GetRectangle() {
  Shader rectangle;
  rectangle.shader = {
      // Bottom layer
      { { -0.5f, -0.5f, 0.0f }, { 1.0f, .0f, .0f }, { 1.0f, 0.0f } },
      { { 0.5f, -0.5f, 0.0f }, { .0f, 1.0f, .0f }, { 0.0f, 0.0f } },
      { { 0.5f, 0.5f, 0.0f }, { .0f, .0f, 1.0f }, { 0.0f, 1.0f } },
      { { -0.5f, 0.5f, 0.0f }, { 1.0f, .0f, 1.0f }, { 1.0f, 1.0f } },

      // Top layer
      { { -0.5f, -0.5f, -0.5f }, { 1.0f, .0f, .0f }, { 1.0f, 0.0f } },
      { { 0.5f, -0.5f, -0.5f }, { .0f, 1.0f, .0f }, { 0.0f, 0.0f } },
      { { 0.5f, 0.5f, -0.5f }, { .0f, .0f, 1.0f }, { 0.0f, 1.0f } },
      { { -0.5f, 0.5f, -0.5f }, { 1.0f, .0f, 1.0f }, { 1.0f, 1.0f } }
  };

  return rectangle;
}

Shader Vertices::GetPent() {
  Shader rectangle;
  rectangle.shader = {
      { { -0.6f, -0.6f, 0.0f }, { 1.0f, .0f, .0f }, { 1.0f, 0.0f } },
      { {  0.0f, -0.8f, 0.0f }, { 1.0f, .0f, .0f }, { 0.0f, 1.0f } },
      { { 0.6f, -0.6f, 0.0f }, { 1.0f, .0f, .0f }, { 1.0f, 1.0f } },
      { { -0.4f, 0.6f, 0.0f }, { 1.0f, .0f, .0f }, { 1.0, 0.0f } },
      { { 0.4f, 0.6f, 0.0f }, { 1.0f, .0f, .0f }, { 0.0f, 1.0f } }
  };

  return rectangle;
}
