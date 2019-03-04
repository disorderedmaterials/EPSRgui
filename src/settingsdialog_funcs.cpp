#include "settingsdialog.h"
#include "epsrproject.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QString>
#include <QFile>

SettingsDialog::SettingsDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);

    mainWindow_ = parent;

    connect(ui.okButton, SIGNAL(clicked()), this, SLOT(writeSettingsFile()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    readSettings();
}

void SettingsDialog::readSettings()
{
    QSettings settings;

    ui.epsrBinDirlineEdit->setText(settings.value("EPSRbindir").toString());
    ui.epsrDirlineEdit->setText(settings.value("EPSRdir").toString());
    ui.visualiserLineEdit->setText(settings.value("visualiser").toString());
    if (settings.value("fmoleReminder").toInt() == 0)
    {
        ui.fmoleReminderCheckBox->setChecked(false);
    }
    else (ui.fmoleReminderCheckBox->setChecked(true));
}

void SettingsDialog::on_okButton_clicked(bool checked)
{

}

void SettingsDialog::on_cancelButton_clicked(bool checked)
{

}

void SettingsDialog::on_browseEPSRDirButton_clicked(bool checked)
{
    QString prefEPSRdir;
    if (ui.epsrDirlineEdit->text().isEmpty())
    {
        prefEPSRdir = QFileDialog::getExistingDirectory(this, "Choose EPSR directory", currentDir.path());
    }
    else
    {
        prefEPSRdir = QFileDialog::getExistingDirectory(this, "Choose EPSR directory", ui.epsrDirlineEdit->text());
    }

    if (!prefEPSRdir.isEmpty())
    {
        ui.epsrDirlineEdit->setText(prefEPSRdir);
    }
}

void SettingsDialog::on_browseEPSRBinDirButton_clicked(bool checked)
{
    QString prefEPSRbindir;
    if (ui.epsrBinDirlineEdit->text().isEmpty())
    {
        prefEPSRbindir = QFileDialog::getExistingDirectory(this, "Choose EPSR bin directory", currentDir.path());
    }
    else
    {
        prefEPSRbindir = QFileDialog::getExistingDirectory(this, "Choose EPSR bin directory", ui.epsrBinDirlineEdit->text());
    }

    if (!prefEPSRbindir.isEmpty())
    {
        ui.epsrBinDirlineEdit->setText(prefEPSRbindir);
    }
}

void SettingsDialog::on_browseVisualiserExeButton_clicked(bool checked)
{
#ifdef _WIN32
    QString prefVisExe = QFileDialog::getOpenFileName(this, "Choose visualiser exectuable", currentDir.path(), tr(".exe files (*.exe)"));
#else
    QString prefVisExe = QFileDialog::getOpenFileName(this, "Choose visualiser exectuable", currentDir.path(), tr("All files (*.*)"));
#endif

    if (!prefVisExe.isEmpty())
    {
        ui.visualiserLineEdit->setText(prefVisExe);
    }
}

void SettingsDialog::writeSettingsFile()
{
    if (ui.epsrBinDirlineEdit->text().isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setText("The EPSR binaries directory needs to be specified in order to use EPSRgui.");
        msgBox.exec();
        return;
    }

    if (!ui.epsrBinDirlineEdit->text().contains("bin"))
    {
        QMessageBox msgBox;
        msgBox.setText("Check that the EPSR binaries path is to the EPSR bin folder");
        msgBox.exec();
        return;
    }

    if (QDir(ui.epsrBinDirlineEdit->text()).exists() == false)
    {
        QMessageBox msgBox;
        msgBox.setText("Check that the EPSR binaries path is correct.");
        msgBox.exec();
        return;
    }

    QSettings settings;
    settings.setValue("EPSRbindir", ui.epsrBinDirlineEdit->text());
    if (!ui.epsrDirlineEdit->text().isEmpty())
    {
        settings.setValue("EPSRdir", ui.epsrDirlineEdit->text());
    }
    else
    {
        settings.remove("EPSRdir");
    }
    if (!ui.visualiserLineEdit->text().isEmpty())
    {
        settings.setValue("visualiser", ui.visualiserLineEdit->text());
    }
    else
    {
        settings.remove("visualiser");
    }
    if (ui.fmoleReminderCheckBox->isChecked())
    {
        settings.setValue("fmoleReminder", 1);
    }
    else
    {
        settings.setValue("fmoleReminder", 0);
    }
    settings.sync();

    accept();
}
