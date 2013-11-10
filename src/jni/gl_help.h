#pragma once

#include "android.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cassert>

static int gl_help_config_weight(EGLConfig const & config, EGLDisplay display)
{
  int val = -1;
  eglGetConfigAttrib(display, config, EGL_CONFIG_CAVEAT, &val);

  switch (val) {
  case EGL_NONE:
    return 0;
  case EGL_SLOW_CONFIG:
    return 1;
  case EGL_NON_CONFORMANT_CONFIG:
    return 2;
  default:
    return 0;
  }
}

int gl_help_compare_config(EGLConfig const & l, EGLConfig const & r, EGLDisplay display)
{
  return gl_help_config_weight(l, display) - gl_help_config_weight(r, display);
}

