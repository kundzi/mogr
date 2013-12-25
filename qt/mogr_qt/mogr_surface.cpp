#include "mogr_surface.hpp"

MogrSurface::MogrSurface(QWindow * parent)
  : QWindow(parent)
{
  setSurfaceType(QWindow::OpenGLSurface);
}

void MogrSurface::exposeEvent(QExposeEvent *event)
{
  Q_UNUSED(event);

  if (isExposed())
    emit windowExposed(this);
}
