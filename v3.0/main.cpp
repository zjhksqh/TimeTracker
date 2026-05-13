#include <QApplication>
#include "database.h"
#include "mainwindow.h"
#include "tracker.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    Database db;
    if (!db.init()) {
        qCritical() << "Database initialization failed!";
        return -1;
    }

    Tracker tracker(&db);
    tracker.start();

    MainWindow w(&db);
    w.show();

    int ret = app.exec();

    tracker.stop();
    return ret;
}
