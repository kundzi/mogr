#pragma once

#include <QWindow>

class MogrSurface : public QWindow
{
  Q_OBJECT
public:
  MogrSurface(QWindow * parent = 0);

signals:
  void windowExposed(QWindow * surface);

protected:
  void exposeEvent(QExposeEvent *event);
};

