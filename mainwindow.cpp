#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "db/DatabaseManager.hpp"

MainWindow::MainWindow(DatabaseManager& dbManager, QWidget *parent)
    : m_dbManager(dbManager), QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
