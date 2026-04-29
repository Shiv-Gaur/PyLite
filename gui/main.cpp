

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PyLite IDE");
    app.setApplicationVersion("0.2");

    MainWindow window;
    window.resize(1200, 700);
    window.show();

    return app.exec();
}
