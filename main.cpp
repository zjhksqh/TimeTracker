#include <QApplication>
#include "MainWindow.h"
#include "DatabaseManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    DatabaseManager::instance().initialize();

    MainWindow w;
    w.show();

    return app.exec();
}
