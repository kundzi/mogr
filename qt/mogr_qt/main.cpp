#include "mogrwindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MogrWindow w;
  w.show();

  return a.exec();
}
