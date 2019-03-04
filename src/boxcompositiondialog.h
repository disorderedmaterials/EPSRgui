#ifndef BOXCOMPOSITION_H
#define BOXCOMPOSITION_H

#include <QDialog>
#include <QWidget>
#include "ui_boxcompositiondialog.h"

class MainWindow;

class BoxCompositionDialog : public QDialog
{
    Q_OBJECT

private:
    MainWindow *mainWindow_;

private slots:
    void on_closeButton_clicked(bool checked);

public:
    BoxCompositionDialog(MainWindow *parent = 0);
    Ui::boxCompositionDialog ui;
};

#endif
