#include "glrenderer.hpp"

#include <QDebug>
#include <QThread>
#include <QOpenGLContext>

#import "../../src/jni/shaders.h"
#import "../../src/jni/transformations.hpp"

#import <string>
using std::string;


#define GL_CHECK(x) { x; \
  GLenum error = glGetError(); \
  if (error != GL_NO_ERROR) \
    { \
      qDebug() << "GL ERROR:" << error; \
      Q_ASSERT(false); \
    } \
}

#define GL_CHECK_AFTER()  { \
  GL_CHECK(reinterpret_cast<int>(0)); \
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

  QOpenGLContext * m_drawContext;
  QOpenGLContext * m_updateContext;
}

void GlRenderer::Start(QWindow * surface)
{
  doLogDraw = true;
  doLogUpdate = true;
  doUpdate = true;

  if (!_isRunning)
  {
    m_surface = surface;
    qDebug() << "INITIALAZING PTHREAD";

    _isRunning = true;

    // create sync objects
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);

    // create joinable threads
    pthread_attr_init(&_attrs);
    pthread_attr_setdetachstate(&_attrs, PTHREAD_CREATE_JOINABLE);

    pthread_create(&_mainThread, &_attrs, GlRenderer::_runMainThread, (void*)this);

    // because for strange dispatcher we have to wait
    usleep(1000*1000);

    pthread_create(&_updateThread, &_attrs, GlRenderer::_runUpdateThread, (void*)this);

    qDebug() << "PTHREAD OK";
  }
}

void GlRenderer::Stop()
{
  if (_isRunning)
  {
    qDebug() << "STOPPING PTHREAD";

    _isRunning = false;

    pthread_join(_mainThread, NULL);
    pthread_join(_updateThread, NULL);

    pthread_attr_destroy(&_attrs);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    qDebug() << "PTHREAD STOPPED";
  }
}

void GlRenderer::RunMainThread()
{
  m_drawContext = new QOpenGLContext();
  m_drawContext->setFormat(m_surface->requestedFormat());
  Q_ASSERT(m_drawContext->create());
  Q_ASSERT(m_drawContext->isValid());


  usleep(500 * 1000);

  qDebug() << "MAIN CONTEXT OK";

  // wait before update thread initialize resources
  pthread_mutex_lock(&_mutex);
  qDebug() << "WAITING FOR UPDATE THREAD";
  pthread_cond_wait(&_cond, &_mutex);
  qDebug() << "MAIN THREAD RESUMED";
  pthread_mutex_unlock(&_mutex);

  m_drawContext->makeCurrent(m_surface);

  qDebug() << "Shared for draw : " << m_drawContext->shareContext();

  qDebug() << "Draw context : " << m_drawContext;
  qDebug() << "Upload context : " << m_updateContext;

  QOpenGLContextGroup * group = QOpenGLContextGroup::currentContextGroup();
  QList<QOpenGLContext *> list = group->shares();
  for (int i = 0; i < list.size(); ++i)
    qDebug() << "Shared context : " << list[i];

  //{@
  GL_CHECK(glUseProgram(_programId));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _bufferId));

  GLint posAttrLoc =  glGetAttribLocation(_programId, "position");
  GL_CHECK_AFTER();
  Q_ASSERT(posAttrLoc != -1);

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
  m_updateContext = new QOpenGLContext();
  m_updateContext->setShareContext(m_drawContext);
  m_updateContext->setFormat(m_surface->requestedFormat());
  Q_ASSERT(m_updateContext->create());
  Q_ASSERT(m_updateContext->isValid());
  m_updateContext->makeCurrent(m_surface);


  Init();
  pthread_cond_broadcast(&_cond);
  qDebug() << "RESOURSES OK";

  while (_isRunning)
  {
    Update();
    usleep(UPDATERATE);
  }
}

void * GlRenderer::_runMainThread(void * thiz)
{
  Q_ASSERT(thiz);
  ((GlRenderer*)thiz)->RunMainThread();
  return 0;
}

void * GlRenderer::_runUpdateThread(void * thiz)
{
  Q_ASSERT(thiz);
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

    qDebug() << "SHADER COMPILE ERROR: " << string(buff, lengh).c_str();
    Q_ASSERT(false);
  }

  return shaderId;
}


void GlRenderer::LinkProgram()
{
  _programId = glCreateProgram();
  GLenum e = glGetError();
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

    qDebug() << "PROGRAM LINK ERROR: " << string(buff, lengh).c_str();
    Q_ASSERT(false);
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
    qDebug() << "DRAWN";
    doLogDraw = false;
  }

  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  GL_CHECK(glViewport(0,0, width, height));

  GL_CHECK(glUseProgram(_programId));

  GLint colorPos = glGetUniformLocation(_programId, "color");
  GL_CHECK_AFTER();
  Q_ASSERT(colorPos != -1);

  GL_CHECK(glUniform4f(colorPos, 1.0, 0.0, 0.0, 1.0));

  GLint modelViewLoc = glGetUniformLocation(_programId, "modelViewMatrix");
  GL_CHECK_AFTER();
  Q_ASSERT(modelViewLoc != -1);

  GLint projectionLoc = glGetUniformLocation(_programId, "projectionMatrix");
  GL_CHECK_AFTER();
  Q_ASSERT(projectionLoc != -1);

  //float aspect = (float)width/height;
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

  m_drawContext->makeCurrent(m_drawContext->surface());
  m_drawContext->swapBuffers(m_drawContext->surface());

  GL_CHECK(glUseProgram(0));
}

void GlRenderer::Update()
{
  if (doLogUpdate)
  {
    qDebug() << "UPDATE";
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

