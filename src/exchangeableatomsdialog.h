#ifndef EXCHANGEABLEATOMSDIALOG_H
#define EXCHANGEABLEATOMSDIALOG_H

#include <QDialog>
#include <QWidget>
#include "ui_exchangeableatomsdialog.h"

class MainWindow;

class ExchangeableAtomsDialog : public QDialog
{
    Q_OBJECT

private:
    MainWindow *mainWindow_;

public slots:
    void on_addButton_clicked(bool checked);
    void on_cancelButton_clicked(bool checked);
    void check();

public:
    ExchangeableAtomsDialog(MainWindow *parent = 0);
    Ui::exchangeableAtomsDialog ui;

    QString getWtsFile();
};

#endif
