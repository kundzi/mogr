//
//  renderer.h
//  mogr_ios
//
//  Created by Dmitry Kunin on 23.12.13.
//  Copyright (c) 2013 mapswithme. All rights reserved.
//

#ifndef __mogr_ios__renderer__
#define __mogr_ios__renderer__

#include <iostream>

#import <QuartzCore/CAEAGLLayer.h>
#import <OpenGLES/EAGL.h>

class GlRenderer
{
private:
  bool _isRunning;
  
  //{@ threading
  void RunMainThread();
  void RunUpdateThread();
  
  static void * _runMainThread(void * thiz);
  static void * _runUpdateThread(void * thiz);
  
  pthread_attr_t _attrs;
  pthread_t _mainThread;
  pthread_t _updateThread;
  
  pthread_cond_t _cond;
  pthread_mutex_t _mutex;
  //}@ threading
  
  //{@ openGl
  CAEAGLLayer * m_layer;
  
  GLuint m_frameBuffId;
  GLuint m_renderBuffId;
  GLuint m_depthBuffId;
  
  GLuint _bufferId;
  GLuint _programId;
  //}@ opengl
  
public:
  int width;
  int height;
  bool doLogUpdate;
  bool doLogDraw;
  bool doUpdate;
  
  GlRenderer()
  : _isRunning(false),
  _mainThread(0),
  _updateThread(0),
  _bufferId(0),
  _programId(0),
  width(400),
  height(400),
  doLogUpdate(true),
  doLogDraw(true),
  doUpdate(true)
  {}
  
  ~GlRenderer()
  {
    Stop();
  }
  
  void Start(CAEAGLLayer * layer);
  void Stop();
  
  void Init();
  void Draw();
  void Update();
  
  GLuint LoadShader(const char * shaderSrc, GLenum type);
  void   LinkProgram();
};

#endif /* defined(__mogr_ios__renderer__) */
