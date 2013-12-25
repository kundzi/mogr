#include "mogrwindow.hpp"
#include "mogr_surface.hpp"

#include <QDebug>
#include <QThread>
#include <QWindow>
#include <QApplication>

MogrWindow::MogrWindow(QWidget *parent)
  : QMainWindow(parent)
{
  m_renderer = new GlRenderer();

  resize(800, 600);
  MogrSurface * surface = new MogrSurface();
  QObject::connect(surface, SIGNAL(windowExposed(QWindow *)), this, SLOT(windowExposed(QWindow *)));
  QWidget * widget = QWidget::createWindowContainer(surface, this);
  setCentralWidget(widget);
}

MogrWindow::~MogrWindow()
{
}

void MogrWindow::resizeEvent(QResizeEvent * e)
{
}

void MogrWindow::windowExposed(QWindow * surface)
{
  if (!m_renderer->IsStarted())
    m_renderer->Start(surface);
}
