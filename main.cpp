#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DatabaseManager dbManager("main_connection");

    if (!dbManager.connect("localhost", "finance", "finance_app", "password")) {
        QMessageBox::critical(nullptr, "Error", "Could not connect to database.");
        return 1;
    }

    if (!dbManager.runMigrations()) {
        QMessageBox::critical(nullptr, "Error", "Migration failed.");
        return 1;
    }

    MainWindow w(dbManager);
    w.show();
    return QCoreApplication::exec();
}
