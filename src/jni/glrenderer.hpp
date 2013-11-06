#pragma once

#include <jni.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <pthread.h>

class GlRenderer
{
private:
  bool _isRunning;
  JavaVM * _jvm;

  //{@ egl
  EGLConfig  _eglConfig;
  EGLContext _eglContext;
  EGLSurface _eglSurface;
  EGLDisplay _eglDisplay;

  EGLContext _eglSharedContext;
  //}@ egl


  //{@ threading
  void RunMainThread();
  void RunUpdateThread();

  static void * _runMainThread(void * thiz);
  static void * _runUpdateThread(void * thiz);

  pthread_t _mainThread;
  pthread_t _updateThread;

  pthread_cond_t _cond;
  pthread_mutex_t _mutex;
  //}@ threading


public:
  GlRenderer(JavaVM * jvm)
    : _isRunning(false), _jvm(jvm),
      _eglConfig(0),
      _eglContext(EGL_NO_CONTEXT),
      _eglSurface(EGL_NO_SURFACE),
      _eglDisplay(EGL_NO_DISPLAY),
      _eglSharedContext(EGL_NO_CONTEXT),
      _mainThread(0),
      _updateThread(0)
    {}

  void Start();
  void Stop();
  void SetUpEgl(EGLConfig  eglConfig,  EGLContext eglContext,
                EGLSurface eglSurface, EGLDisplay eglDisplay);
};
