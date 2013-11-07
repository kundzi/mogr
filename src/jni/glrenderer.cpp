#include "glrenderer.hpp"
#include "transformations.hpp"
#include "android.hpp"

#include <assert.h>
#include <unistd.h>

void GlRenderer::Start()
{

  if (!_isRunning)
  {
    LOG("INITIALAZING PTHREAD");

    assert(_eglContext != EGL_NO_CONTEXT);
    assert(_eglSurface != EGL_NO_SURFACE);
    assert(_eglDisplay != EGL_NO_DISPLAY);

    _isRunning = true;

    // create sync objects
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);

    // create joinable threads
    pthread_attr_init(&_attrs);
    pthread_attr_setdetachstate(&_attrs, PTHREAD_CREATE_JOINABLE);

    pthread_create(&_mainThread, &_attrs, GlRenderer::_runMainThread, (void*)this);
    pthread_create(&_updateThread, &_attrs, GlRenderer::_runUpdateThread, (void*)this);

    LOG("PTHREAD OK");
  }
}

void GlRenderer::Stop()
{
  if (_isRunning)
  {
    LOG("STOPPING PTHREAD");

    _isRunning = false;

    pthread_join(_mainThread, NULL);
    pthread_join(_updateThread, NULL);

    pthread_attr_destroy(&_attrs);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    LOG("PTHREAD STOPPED");
  }
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

  // wait before update thread initialize resources
  pthread_mutex_lock(&_mutex);
  LOG("WAITING FOR UPDATE THREAD");
  pthread_cond_wait(&_cond, &_mutex);
  LOG("MAIN THREAD RESUMED");
  pthread_mutex_unlock(&_mutex);

  while (_isRunning)
  {
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
  LOG("SHARED CONTEXT OK");

  //simulate resource initialization
  pthread_mutex_lock(&_mutex);
  LOG("START INIT RESOURSES");
  sleep(3);
  pthread_cond_broadcast(&_cond);
  LOG("RESOURSES OK");
  pthread_mutex_unlock(&_mutex);

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
