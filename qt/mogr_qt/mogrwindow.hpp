#ifndef MOGRWINDOW_HPP
#define MOGRWINDOW_HPP

#include "glrenderer.hpp"

#include <QMainWindow>
#include <QOpenGLContext>
#include <QSurface>
#include <QTimerEvent>
#include <QResizeEvent>

class MogrWindow : public QMainWindow
{
  Q_OBJECT

public:
  MogrWindow(QWidget *parent = 0);
  ~MogrWindow();

protected:
  void resizeEvent(QResizeEvent * e);

  Q_SLOT void windowExposed(QWindow *surface);

private:
  GlRenderer * m_renderer;
};

#endif // MOGRWINDOW_HPP
