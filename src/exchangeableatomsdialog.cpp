#include "exchangeableatomsdialog.h"
#include "epsrproject.h"

#include <QDialog>
#include <QWidget>
#include <QPushButton>

ExchangeableAtomsDialog::ExchangeableAtomsDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);
    mainWindow_ = parent;

    QStringList wtsFiles = mainWindow_->listWtsFiles();
    QString wtsBaseFileName = mainWindow_->wtsBaseFileName();
    ui.weightsSetupFileList->clear();

    for (int i = 0; i < wtsFiles.count(); i++)
    {
        if (wtsFiles.at(i).contains(".") && !wtsFiles.at(i).contains(wtsBaseFileName))
        {
            ui.weightsSetupFileList->addItem(wtsFiles.at(i).split(".", QString::SkipEmptyParts).at(0)+".NWTS.dat");
        }
    }
    ui.weightsSetupFileList->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui.addButton, SIGNAL(clicked()), this, SLOT(check()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void ExchangeableAtomsDialog::on_addButton_clicked(bool checked)
{

}

void ExchangeableAtomsDialog::on_cancelButton_clicked(bool checked)
{

}

void ExchangeableAtomsDialog::check()
{
    if (ui.weightsSetupFileList->currentRow() == -1)
    {
        QMessageBox msgBox;
        msgBox.setText("First select a weights file to use.");
        msgBox.exec();
        return;
    }

    getWtsFile();
    accept();
}

QString ExchangeableAtomsDialog::getWtsFile()
{
    QString wtsFile = ui.weightsSetupFileList->item(ui.weightsSetupFileList->currentRow())->text();
    return wtsFile;
}
