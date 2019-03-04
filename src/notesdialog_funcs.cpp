#include "notesdialog.h"
#include "epsrproject.h"

#include <QPushButton>
#include <QTextStream>

NotesDialog::NotesDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);

    mainWindow_ = parent;
    workingDir_ = mainWindow_->workingDir();

    connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(save()));
    connect(ui.saveCloseButton, SIGNAL(clicked()), this, SLOT(closeDialog()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    Qt::WindowFlags flags = Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint;
    this->setWindowFlags(flags);

    ui.notesTextEdit->clear();
    readNotes();
}

void NotesDialog::on_saveButton_clicked(bool checked)
{

}

void NotesDialog::on_saveCloseButton_clicked(bool checked)
{

}

void NotesDialog::on_cancelButton_clicked(bool checked)
{

}

void NotesDialog::readNotes()
{
    QFile file(workingDir_+"/notes.txt");
    if (file.exists())
    {
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream stream(&file);
        QString line;
        do {
            line = stream.readLine();
            ui.notesTextEdit->append(line);
        } while (!line.isNull());
        file.close();
    }
}

bool NotesDialog::save()
{
    QFile file(workingDir_+"/notes.txt");
    if (!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not write to notes file");
        msgBox.exec();
        return false;
    }
    QTextStream stream(&file);
    QString comments = ui.notesTextEdit->toPlainText();
    stream << comments;
    file.close();
    return true;
}

void NotesDialog::closeDialog()
{
    save();
    if (save() == true)
    {
        close();
    }
}
