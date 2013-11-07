#pragma once

namespace Shaders
{
  char const * Fragment = "uniform lowp vec4 color;"
                          "void main(void)"
                          "{"
                          "   gl_FragColor = color;"
                          "}";

  char const * Vertex =  "attribute mediump vec2 position;"
                         "uniform mediump mat4 modelViewMatrix;"
                         "uniform mediump mat4 projectionMatrix;"
                         "void main(void)"
                         "{"
                             "gl_Position = vec4(position, 0.0, 1.0) * modelViewMatrix * projectionMatrix;"
                         "}";
}
