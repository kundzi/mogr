#ifndef MOGRWINDOW_HPP
#define MOGRWINDOW_HPP

#include <QMainWindow>

class MogrWindow : public QMainWindow
{
  Q_OBJECT

public:
  MogrWindow(QWidget *parent = 0);
  ~MogrWindow();
};

#endif // MOGRWINDOW_HPP
