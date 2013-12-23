//
//  renderer.cpp
//  mogr_ios
//
//  Created by Dmitry Kunin on 23.12.13.
//  Copyright (c) 2013 mapswithme. All rights reserved.
//

#include "renderer.h"

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "../../../src/jni/shaders.h"
#import "../../../src/jni/transformations.hpp"

#import <string>
using std::string;


#define GL_CHECK(x) { x; \
GLenum error = glGetError(); \
if (error != GL_NO_ERROR) \
{ \
NSLog(@"GL ERROR: %i", (int)error); \
assert(false); \
} \
}

#define GL_CHECK_AFTER()  { \
GL_CHECK((uint)(0)); \
}


namespace {
  const float _vertexData[4 /*vertexes*/ * 2 /*dimensions*/] =
  {
    -1.5, -1.5,
    0.5, -1.5,
    -1.5,  0.5,
    0.5,  0.5
  };
  
  const long FRAMERATE = 1000*1000/30;
  const long UPDATERATE = 1000*1000/30;
  
  EAGLContext * m_drawContext;
  EAGLContext * m_updateContext;
}

void GlRenderer::Start(CAEAGLLayer * layer)
{
  m_layer = layer;
  
  doLogDraw = true;
  doLogUpdate = true;
  doUpdate = true;
  
  m_drawContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
  m_updateContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2 sharegroup:m_drawContext.sharegroup];
  
  if (!_isRunning)
  {
    NSLog(@"INITIALAZING PTHREAD");
    
    _isRunning = true;
    
    // create sync objects
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    
    // create joinable threads
    pthread_attr_init(&_attrs);
    pthread_attr_setdetachstate(&_attrs, PTHREAD_CREATE_JOINABLE);
    
    pthread_create(&_mainThread, &_attrs, GlRenderer::_runMainThread, (void*)this);
    usleep(1000*1000);
    pthread_create(&_updateThread, &_attrs, GlRenderer::_runUpdateThread, (void*)this);
    
    NSLog(@"PTHREAD OK");
  }
}

void GlRenderer::Stop()
{
  if (_isRunning)
  {
    NSLog(@"STOPPING PTHREAD");
    
    _isRunning = false;
    
    pthread_join(_mainThread, NULL);
    pthread_join(_updateThread, NULL);
    
    pthread_attr_destroy(&_attrs);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
    
    NSLog(@"PTHREAD STOPPED");
  }
}

void GlRenderer::RunMainThread()
{
  NSLog(@"MAIN CONTEXT OK");
  
  // wait before update thread initialize resources
  pthread_mutex_lock(&_mutex);
  NSLog(@"WAITING FOR UPDATE THREAD");
  pthread_cond_wait(&_cond, &_mutex);
  NSLog(@"MAIN THREAD RESUMED");
  pthread_mutex_unlock(&_mutex);
  
  [EAGLContext setCurrentContext: m_drawContext];
  
  glGenRenderbuffers(1, &m_renderBuffId);
  GL_CHECK_AFTER();
  glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffId);
  GL_CHECK_AFTER();
  BOOL res0 = [m_drawContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:m_layer];
  NSLog(@"renderBuffStorage: %i", res0 ? 1 : 0);
  GL_CHECK_AFTER();
  
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
  GL_CHECK_AFTER();
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
  GL_CHECK_AFTER();
  NSLog(@"Size %i x %i", width, height);
  
  glGenRenderbuffers(1, &m_depthBuffId);
  GL_CHECK_AFTER();
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffId);
  GL_CHECK_AFTER();
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
  GL_CHECK_AFTER();
  
  glGenFramebuffers(1, &m_frameBuffId);
  GL_CHECK_AFTER();
  glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffId);
  GL_CHECK_AFTER();
  
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderBuffId);
  GL_CHECK_AFTER();
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffId);
  GL_CHECK_AFTER();
  
  GLenum res = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  NSLog(@"Check Framebuffer: %i, is ok: %i", res, res == GL_FRAMEBUFFER_COMPLETE ? 1 : 0);
  
  GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, m_renderBuffId));

  
  //{@
  GL_CHECK(glUseProgram(_programId));
  
  NSLog(@"VAO IS: %d", _vao);
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _bufferId));
  
  GLint posAttrLoc =  glGetAttribLocation(_programId, "position");
  GL_CHECK_AFTER();
  assert(posAttrLoc != -1);
  
  GL_CHECK(glEnableVertexAttribArray(posAttrLoc));
  GL_CHECK(glVertexAttribPointer(posAttrLoc, 2, GL_FLOAT, GL_FALSE, 0, NULL));
  //}@
  
  // first update must be before first draw
  Update();
  while (_isRunning)
  {
    Draw();
    usleep(FRAMERATE);
  }
}

void GlRenderer::RunUpdateThread()
{
  [EAGLContext setCurrentContext:m_updateContext];
  
  Init();
  pthread_cond_broadcast(&_cond);
  NSLog(@"RESOURSES OK");
  
  while (_isRunning)
  {
    Update();
    usleep(UPDATERATE);
  }
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
    
    NSLog(@"%s", (string("SHADER COMPILE ERROR: ") + string(buff, lengh)).c_str());
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
    
    NSLog(@"%s", (string("PROGRAM LINK ERROR: ") + string(buff, lengh)).c_str());
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
  if (doLogDraw)
  {
    NSLog(@"DRAWN");
    doLogDraw = false;
  }
  
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  GL_CHECK(glViewport(0,0, width, height));
  
  GL_CHECK(glUseProgram(_programId));
  
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
  
  float aspect = (float)width/height;
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
  
  [m_drawContext presentRenderbuffer:GL_RENDERBUFFER];
  
  GL_CHECK(glUseProgram(0));
}

void GlRenderer::Update()
{
  if (doLogUpdate)
  {
    NSLog(@"UPDATE");
    doLogUpdate = false;
  }
  
  static float angle = .0f;
  angle += 0.0125f;
  math::Matrix<float, 3, 3> m = math::Rotate(math::Identity<float, 3>(), angle);
  
  
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
