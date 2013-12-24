#ifndef MOGRVIEW_HPP
#define MOGRVIEW_HPP

#define DESKTOP

#include <QGLWidget>
#include <QDebug>

#include "glrenderer.hpp"

class MogrView : public QGLWidget
{
  Q_OBJECT
public:
  explicit MogrView(QWidget *parent = 0);

  QSize sizeHint() const;

protected:
  void initializeGL() {}
  void paintGL() {}
  void resizeGL(int w, int h)
  {
    qDebug() << "Resize" << w << h;
    m_renderer->width = w;
    m_renderer->height = h;
    m_renderer->Start();
  }

signals:

public slots:


private:
  GlRenderer * m_renderer;
};

#endif // MOGRVIEW_HPP
