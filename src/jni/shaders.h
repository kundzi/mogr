#pragma once

#if defined(DESKTOP)
  #define LOW_P ""
  #define MEDIUM_P ""
#else
  #define LOW_P ""
  #define MEDIUM_P ""
#endif

namespace Shaders
{
  char const * Fragment = "uniform " LOW_P " vec4 color;"
                          "void main(void)"
                          "{"
                          "   gl_FragColor = color;"
                          "}";

  char const * Vertex =  "attribute " MEDIUM_P " vec2 position;"
                         "uniform " MEDIUM_P " mat4 modelViewMatrix;"
                         "uniform " MEDIUM_P " mat4 projectionMatrix;"
                         "void main(void)"
                         "{"
                             "gl_Position = vec4(position, 0.0, 1.0) * modelViewMatrix * projectionMatrix;"
                         "}";
}
