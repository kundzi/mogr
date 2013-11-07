#include "glrenderer.hpp"
#include "transformations.hpp"
#include "android.hpp"
#include "shaders.h"

#include <string>
#include <assert.h>
#include <unistd.h>

#include <GLES2/gl2ext.h>

#include "matrix.hpp"
#include "transformations.hpp"

PFNGLGENVERTEXARRAYSOESPROC _genVertexArray;
PFNGLDELETEVERTEXARRAYSOESPROC _deleteVertexArray;
PFNGLBINDVERTEXARRAYOESPROC _bindVertexArray;

#define GL_CHECK(x) { x; \
  GLenum error = glGetError(); \
  if (error != GL_NO_ERROR) \
  { \
    LOG("GL ERROR: %x", (int)error); \
    assert(false); \
  } \
}

#define GL_CHECK_AFTER()  {\
    int i; \
    GL_CHECK(reinterpret_cast<void*>(i)); \
    }

using std::string;

namespace {
  float _vertexData[4 /*vertexes*/ * 2 /*dimensions*/] =
  {
    -1.5, -1.5,
     0.5, -1.5,
    -1.5,  0.5,
     0.5,  0.5
  };
}

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

  //{@
  GL_CHECK(glUseProgram(_programId));
  GL_CHECK(_genVertexArray(1, &_vao));
  GL_CHECK(_bindVertexArray(_vao));
  LOG("VAO IS: %d", _vao);

  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _bufferId));

  GLint posAttrLoc =  glGetAttribLocation(_programId, "position");
  GL_CHECK_AFTER();
  assert(posAttrLoc != -1);

  GL_CHECK(glEnableVertexAttribArray(posAttrLoc));
  GL_CHECK(glVertexAttribPointer(posAttrLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL));
  GL_CHECK(_bindVertexArray(0));
  //}@

  while (_isRunning)
  {
    Draw();
    assert(eglSwapBuffers(_eglDisplay, _eglSurface) == EGL_TRUE);
    usleep(1000*1000/25);
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
  LOG("START INIT RESOURSES");
  _genVertexArray = reinterpret_cast<PFNGLGENVERTEXARRAYSOESPROC>(eglGetProcAddress("glGenVertexArraysOES"));
  _bindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYOESPROC>(eglGetProcAddress("glBindVertexArrayOES"));


  Init();
  pthread_cond_broadcast(&_cond);
  LOG("RESOURSES OK");

  while (_isRunning)
  {
    Update();
    usleep(1000*1000/25);
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

GLuint GlRenderer::LoadShader(const char * shaderSrc, GLenum type)
{
  GLuint shaderId = glCreateShader(type);
  GL_CHECK_AFTER();

  GL_CHECK(glShaderSource(shaderId, 1, &shaderSrc, NULL));
  GL_CHECK(glCompileShader(shaderId));

  GLint compileResult = GL_FALSE;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileResult);

  if (compileResult != GL_TRUE)
  {
    GLchar buff[1024];
    GLint lengh = 0;
    glGetShaderInfoLog(shaderId, 1024, &lengh, buff);

    LOG("%s", (string("SHADER COMPILE ERROR: ") + string(buff, lengh)).c_str());
    assert(false);
  }

  return shaderId;
}

void GlRenderer::LinkProgram()
{
  _programId = glCreateProgram();
  GL_CHECK_AFTER();

  GLuint vShader = LoadShader(Shaders::Vertex, GL_VERTEX_SHADER);
  GLuint fShader = LoadShader(Shaders::Fragment, GL_FRAGMENT_SHADER);

  GL_CHECK(glAttachShader(_programId, vShader));
  GL_CHECK(glAttachShader(_programId, fShader));

  GL_CHECK(glLinkProgram(_programId));

  GLint linkRes = GL_FALSE;
  glGetProgramiv(_programId, GL_LINK_STATUS, &linkRes);

  if (linkRes != GL_TRUE)
  {
    GLchar buff[1024];
    GLint lengh = 0;
    glGetProgramInfoLog(_programId, 1024, &lengh, buff);

    LOG("%s", (string("PROGRAM LINK ERROR: ") + string(buff, lengh)).c_str());
    assert(false);
  }

  GL_CHECK(glDetachShader(_programId, vShader));
  GL_CHECK(glDetachShader(_programId, fShader));

  GL_CHECK(glDeleteShader(vShader));
  GL_CHECK(glDeleteShader(fShader));
}

void GlRenderer::Init()
{
  LinkProgram();
  GL_CHECK(glGenBuffers(1, &_bufferId));

  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _bufferId));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(_vertexData), _vertexData, GL_DYNAMIC_DRAW));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));


  GL_CHECK(glFlush());
}

void GlRenderer::Draw()
{
  glViewport(0,0, width, height);

  GL_CHECK(glUseProgram(_programId));
  GL_CHECK(_bindVertexArray(_vao));

  GLint colorPos = glGetUniformLocation(_programId, "color");
  GL_CHECK_AFTER();
  assert(colorPos != -1);

  GL_CHECK(glUniform4f(colorPos, 1.0, 0.0, 0.0, 1.0));

  GLint modelViewLoc = glGetUniformLocation(_programId, "modelViewMatrix");
  GL_CHECK_AFTER();
  assert(modelViewLoc != -1);

  GLint projectionLoc = glGetUniformLocation(_programId, "projectionMatrix");
  GL_CHECK_AFTER();
  assert(projectionLoc != -1);

  float modelView[16] =
  {
    -1.0, 0.0,  0.0, 0.0,
     0.0, 1.0,  0.0, 0.0,
     0.0, 0.0, -1.0, 0.0,
     0.0, 0.0, -1.0, 1.0
  };

  float projectionMatrix[16] =
  {
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
  };

  GL_CHECK(glUniformMatrix4fv(modelViewLoc, 1, GL_FALSE, modelView));
  GL_CHECK(glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMatrix));

  GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  GL_CHECK(_bindVertexArray(0));
  GL_CHECK(glUseProgram(0));
}

void GlRenderer::Update()
{
  static float angle = .0f;
  angle += 0.0125f;
  math::Matrix<double, 3, 3> m = math::Rotate(math::Identity<double, 3>(), angle);


  float points[4*2];
  for (int i = 0; i < 8; i+=2)
  {
    points[i] = _vertexData[i] * m(0, 0) + _vertexData[i + 1] * m(1, 0) + m(2, 0);
    points[i + 1] = _vertexData[i] * m(0, 1) + _vertexData[i + 1] * m(1, 1) + m(2, 1);
  }

  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _bufferId));
  GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER ,0, sizeof(points), points));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

  GL_CHECK(glFlush());
}
