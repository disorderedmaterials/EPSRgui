#include "removecomponentdialog.h"
#include "epsrproject.h"

#include <QPushButton>

RemoveComponentDialog::RemoveComponentDialog(MainWindow *parent)
{
    ui.setupUi(this);

    mainWindow_ = parent;
    atoFileList_ = mainWindow_->listAtoFiles();

//    for (int i = 0; i < atoFileList_.count(); i++)
//    {
    ui.componentList->addItems(atoFileList_);
//    }
    ui.componentList->setSelectionMode(QAbstractItemView::SingleSelection);


    connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(check()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

}

void RemoveComponentDialog::on_removeButton_clicked(bool checked)
{

}

void RemoveComponentDialog::on_cancelButton_clicked(bool checked)
{

}

void RemoveComponentDialog::check()
{
    if (ui.componentList->count() < 2)
    {
        QMessageBox msgBox;
        msgBox.setText("This is the only component in the box so cannot be removed");
        msgBox.exec();
        return;
    }

    if (ui.componentList->currentRow() == -1)
    {
        QMessageBox msgBox;
        msgBox.setText("First select a component to remove.");
        msgBox.exec();
        return;
    }

    returnComponent();
    accept();
}

int RemoveComponentDialog::returnComponent()
{
    int componentRow = ui.componentList->currentRow();
    return componentRow;
}
