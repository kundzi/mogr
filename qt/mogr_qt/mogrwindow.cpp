#include "mogrwindow.hpp"
#include "mogrview.hpp"

#include <QDebug>
#include <QThread>

MogrWindow::MogrWindow(QWidget *parent)
  : QMainWindow(parent)
{
  qDebug() << "App thread:" << QThread::currentThread();
  setCentralWidget(new MogrView(this));
}

MogrWindow::~MogrWindow()
{

}
