// local includes
#include "gl_help.h"
#include "glrenderer.hpp"
#include "android.hpp"

// system includes
#include <jni.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <algorithm>

/*
 * Using GLES2
 */
#include <GLES2/gl2.h>
/*
 * Using EGL
 */
#include <EGL/egl.h>
/*
 * And use native_window
 */
#include <android/native_window.h>
#include <android/native_window_jni.h>

extern "C"
{
  ANativeWindow * _nativeWindow;
  EGLDisplay _display;
  EGLSurface _surface;
  EGLContext _context;
  EGLConfig _config;

  GlRenderer * _renderer;

  //{@ DONT TOUCH
  JavaVM * g_JavaVM;

  JNIEXPORT jint JNICALL
  JNI_OnLoad(JavaVM* vm, void* reserved)
  {
    g_JavaVM = vm;
    // TODO: move somewhere else
    _renderer = new GlRenderer();
    return JNI_VERSION_1_6;
  }


  int config_comp(EGLConfig const & l, EGLConfig const & r)
  {
    return gl_help_compare_config(l, r, _display);
  }

  void createEgl(JNIEnv * env, jobject jsurface)
  {
    LOG("CREATING CONTEXT");

    // init egl here
    _nativeWindow = ANativeWindow_fromSurface(env, jsurface);
    LOG("FROM SURFACE");
    assert(_nativeWindow);
    _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    LOG("GOT DISPLAY");
    assert(_display);

    //init egl
    EGLint version[2] = {0};
    assert(eglInitialize(_display,&version[0], &version[1]));
    LOG("INITIALIZED");

    EGLint attr_list[] = {
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 6,
        EGL_BLUE_SIZE, 5,
        EGL_STENCIL_SIZE, 0,
        EGL_DEPTH_SIZE, 16,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLConfig configs[40];
    int num_config = 0;
    assert(eglChooseConfig(_display, attr_list, configs, 40, &num_config));
    LOG("GOT CONFIGS %d", num_config);
    assert(num_config > 0);

    std::sort(&configs[0], &configs[num_config], config_comp);
    LOG("SORTED CONFIGS");

    for (int i = 0; i < num_config; ++i)
    {
        EGLConfig currentConfig = configs[i];
        _surface = eglCreateWindowSurface(_display, currentConfig, _nativeWindow, 0);
        LOG("CREATING WS");

        if (_surface == EGL_NO_SURFACE)
          continue;

        EGLint contextAttrList[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };

        _context = eglCreateContext(_display, currentConfig, EGL_NO_CONTEXT, contextAttrList);
        LOG("CREATING CONT");

        if (_context == EGL_NO_CONTEXT)
        {
            eglDestroySurface(_display, _surface);
            continue;
        }
        _config = currentConfig;
        break;
    }

    // final check
    assert(_surface);
    assert(_context);
    LOG("CONTEXT CREATED");

    _renderer->SetUpEgl(_config, _context, _surface, _display);
    _renderer->Start();
  }

  void destroyEgl(JNIEnv * env)
  {
    _renderer->Stop();

    // destroy egl
    if (_display != EGL_NO_DISPLAY)
    {
        if (_context != EGL_NO_CONTEXT)
          eglDestroyContext(_display, _context);

        if (_surface != EGL_NO_SURFACE)
          eglDestroySurface(_display, _surface);

        eglTerminate(_display);

        LOG("EGL TERMINATED");
    }

    ANativeWindow_release(_nativeWindow);
    _nativeWindow = NULL;
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_nativeOnCreateSurface(JNIEnv * env, jobject thiz, jobject jsurface)
  {
    createEgl(env, jsurface);
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_nativeOnDestroySurface(JNIEnv * env, jobject thiz)
  {
    destroyEgl(env);
  }

  JNIEXPORT void JNICALL
  Java_dk_kunin_pthreadrender_PthreadRender_nativeOnSurfaceChanged(JNIEnv * env, jobject thiz, jint width, jint height)
  {
    _renderer->width = width;
    _renderer->height = height;
  }

} // extern "C"
