#include "glrenderer.hpp"
#include "android.hpp"

#include <assert.h>
#include <unistd.h>

void GlRenderer::Start()
{
  assert(_eglContext != EGL_NO_CONTEXT);
  assert(_eglSurface != EGL_NO_SURFACE);
  assert(_eglDisplay != EGL_NO_DISPLAY);

  _isRunning = true;

  pthread_create(&_mainThread, NULL, GlRenderer::_runMainThread, (void*)this);
  pthread_create(&_updateThread, NULL, GlRenderer::_runUpdateThread, (void*)this);
}

void GlRenderer::Stop()
{
  _isRunning = false;
}

void GlRenderer::SetUpEgl(EGLConfig  eglConfig,  EGLContext eglContext,
                          EGLSurface eglSurface, EGLDisplay eglDisplay)
{
  // simply set, no checks
  _eglConfig  = eglConfig;
  _eglContext = eglContext;
  _eglSurface = eglSurface;
  _eglDisplay = eglDisplay;
}

void GlRenderer::RunMainThread()
{
  assert(eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext));
  LOG("MAIN CONTEXT OK");

  while (_isRunning)
  {
//    sleep(1);
    //LOG("MAIN THREAD LOOP");
  }

  eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  LOG("MAIN CONTEXT UNBINDED");
}

void GlRenderer::RunUpdateThread()
{
  // bind to context
  EGLint contextAttrList[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  _eglSharedContext = eglCreateContext(_eglDisplay,_eglConfig, _eglContext, contextAttrList);
  assert(_eglSharedContext != EGL_NO_CONTEXT);
  assert(eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, _eglSharedContext));
  LOG("Shared context OK");

  while (_isRunning)
  {
//    sleep(1);
    //LOG("UPDATE THREAD LOOP");
  }

  // unbind context
  eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(_eglDisplay, _eglSharedContext);
  LOG("SHARED CONTEXT TERMINATED");
}

void * GlRenderer::_runMainThread(void * thiz)
{
  assert(thiz);
  ((GlRenderer*)thiz)->RunMainThread();
  return 0;
}

void * GlRenderer::_runUpdateThread(void * thiz)
{
  assert(thiz);
  ((GlRenderer*)thiz)->RunUpdateThread();
  return 0;
}
