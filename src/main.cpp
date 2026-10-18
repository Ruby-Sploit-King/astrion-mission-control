#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  QApplication::setApplicationName("ASTRION");
  QApplication::setApplicationVersion("1.0.0");
  QApplication::setOrganizationName("ASTRION Project");

  MainWindow window;
  window.show();

  return app.exec();
}

