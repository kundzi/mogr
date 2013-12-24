#include "mogrview.hpp"

MogrView::MogrView(QWidget *parent) :
  QGLWidget(parent)
{
  setAutoBufferSwap(false);
  m_renderer = new GlRenderer(this);
}

QSize MogrView::sizeHint() const
{
    return QSize(800, 800);
}


