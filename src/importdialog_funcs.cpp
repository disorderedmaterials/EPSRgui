#include "importdialog.h"
#include "epsrproject.h"

#include <QPushButton>
#include <QFileDialog>

ImportDialog::ImportDialog(MainWindow *parent) : QDialog(parent)
{
    ui.setupUi(this);

    mainWindow_ = parent;
    epsrDir_ = mainWindow_->epsrDir();

    connect(ui.importButton, SIGNAL(clicked()), this, SLOT(import()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void ImportDialog::on_browseFolderButton_clicked(bool checked)
{
    QString newDir = QFileDialog::getExistingDirectory(this, "Choose simulation directory", epsrDir_);
    if (!newDir.isEmpty())
    {
        //get project name from path
        QStringList pathList = newDir.split("/", QString::SkipEmptyParts);
        QString projName = pathList.at(pathList.count()-1);
        QString baseDir = newDir.split(projName, QString::SkipEmptyParts).at(0);

        //check if an <project name>.EPSR.pro file already exists in folder
        QFile file(newDir+"/"+projName+".EPSR.pro");
        if (file.exists())
        {
            QMessageBox msgBox;
            msgBox.setText("An EPSR.pro file already exists for this simulation.\n");
            msgBox.exec();
            return;
        }

        //assign directories etc
        workingDir_ = newDir+"/";
        workingDir_ = QDir::toNativeSeparators(workingDir_);
        baseDir = QDir::toNativeSeparators(baseDir);
        ui.folderLineEdit->setText(workingDir_);
        ui.importToFolderLineEdit->setText(baseDir);
        projectName_ = projName;
        ui.newProjectNameLineEdit->setText(projectName_);
    }
}

void ImportDialog::on_browseBoxButton_clicked(bool checked)
{
    QString boxFileName;
    if (!workingDir_.isEmpty())
    {
        boxFileName = QFileDialog::getOpenFileName(this, "Choose simulation box .ato file", workingDir_, tr("box .ato files (*.ato)"));
        if (!boxFileName.isEmpty())
        {
            atoFileName_ = QFileInfo(boxFileName).fileName();
            ui.boxLineEdit->setText(atoFileName_);
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("The project folder needs to be defined first.");
        msgBox.exec();
        return;
    }
}

void ImportDialog::on_browseInpButton_clicked(bool checked)
{
    QString inpFileName;
    if (!workingDir_.isEmpty())
    {
        inpFileName = QFileDialog::getOpenFileName(this, "Choose simulation input file", workingDir_, tr(".EPSR.inp files (*.inp)"));
        if (!inpFileName.isEmpty())
        {
            epsrInpFileName_ = QFileInfo(inpFileName).fileName();
            ui.inpLineEdit->setText(epsrInpFileName_);
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("The project folder needs to be defined first.");
        msgBox.exec();
        return;
    }
}

void ImportDialog::on_browseScriptButton_clicked(bool checked)
{
    QString scriptFileName;
    if (!workingDir_.isEmpty())
    {
        scriptFileName = QFileDialog::getOpenFileName(this, "Choose simulation script file", workingDir_, tr(".txt files (*.txt)"));
        if (!scriptFileName.isEmpty())
        {
            scriptFile_ = QFileInfo(scriptFileName).fileName();
            ui.scriptLineEdit->setText(scriptFile_);
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("The project folder needs to be defined first.");
        msgBox.exec();
        return;
    }
}

void ImportDialog::on_addComponentButton_clicked(bool checked)
{

}

void ImportDialog::on_removeComponentButton_clicked(bool checked)
{

}

void ImportDialog::on_browseNewFolderButton_clicked(bool checked)
{
    QString newDir = QFileDialog::getExistingDirectory(this, "Choose directory for simulation to be imported to", epsrDir_);
    if (!newDir.isEmpty())
    {
        //assign directories etc
        newWorkingDir_ = newDir+"/";
        newWorkingDir_ = QDir::toNativeSeparators(newWorkingDir_);
        ui.importToFolderLineEdit->setText(newWorkingDir_);
    }
}

void ImportDialog::on_importButton_clicked(bool checked)
{

}

void ImportDialog::on_cancelButton_clicked(bool checked)
{

}

void ImportDialog::import()
{
    //get original simultion directory and new simulation directory
    workingDir_ = ui.folderLineEdit->text();
    workingDir_ = QDir::toNativeSeparators(workingDir_);
    projectName_ = ui.newProjectNameLineEdit->text();
    newWorkingDir_ = ui.importToFolderLineEdit->text()+projectName_+"/";
    newWorkingDir_ = QDir::toNativeSeparators(newWorkingDir_);

    //check original simulation directory path is correct
    if (!QDir(workingDir_).exists())
    {
        QMessageBox msgBox;
        msgBox.setText("Check the path of the simulation to be imported.");
        msgBox.exec();
        return;
    }

    //check if an EPSR.pro file is already in the directory
    QFile proFile(workingDir_+projectName_+".EPSR.pro");
    QFile newProFile(newWorkingDir_+projectName_+".EPSR.pro");
    if (proFile.exists() || newProFile.exists())
    {
        QMessageBox msgBox;
        msgBox.setText("An EPSR.pro file already exists for this simulation.");
        msgBox.exec();
        return;
    }

    if (newWorkingDir_ != workingDir_) //this can be a change in path or projectName_
    {
        //check destination path is viable i.e. a simulation of the same name doesn't already exist there
        if (QDir(newWorkingDir_).exists())
        {
            QMessageBox msgBox;
            msgBox.setText("A folder already exists in this directory for this simulation.");
            msgBox.exec();
            return;
        }

        //make new directory and make a list of everything in the current directory
        QDir().mkdir(newWorkingDir_);
        QDir workingDir(workingDir_);
        QStringList fileList = workingDir.entryList(QDir::Files);

        //copy everything into new folder
        for (int i = 0; i < fileList.count(); i++)
        {
            QFile::copy(workingDir_+fileList.at(i), newWorkingDir_+fileList.at(i));
        }

        //make the newWorkingDir_ the workingDir_
        workingDir_ = newWorkingDir_;
    }

    QRegExp ecoredcorerx("  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})  ([0-9]{1}[.]{1}[0-9]{1,}[E+-]{2}[0-9]{1,})");
    QStringList molFileList;
    QStringList dataFileList;
    QStringList wtsFileList;
    QStringList nrtypeList;
    QStringList normalisationList;
    molFileList.clear();
    dataFileList.clear();
    wtsFileList.clear();
    nrtypeList.clear();
    normalisationList.clear();

    //read box .ato file to get components and number of each
    if (!atoFileName_.isEmpty())
    {
        QFile fileato(workingDir_+atoFileName_);
        if(!fileato.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open .ato file.");
            msgBox.exec();
            return;
        }

        QTextStream stream(&fileato);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        do {
            line = stream.readLine();
        } while (!ecoredcorerx.exactMatch(line));

        line = stream.readLine(); //this is the line of random(?) large numbers
        do {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() == 0) break;
            if (dataLine.at(0) == "Atomic") break;
            molFileList.append(dataLine.at(1));
        } while (!line.isNull());

        fileato.close();
    }

    //check molFileList has been populated
    if(molFileList.count() == 0)
    {
        QMessageBox msgBox;
        msgBox.setText("Could not read components from .ato file.");
        msgBox.exec();
        return;
    }

    //check if any of the components are "moltype"
    for (int i = 0; i < molFileList.count(); i++)
    {
        if (molFileList.at(i).contains("moltype"))
        {
            QMessageBox msgBox;
            msgBox.setText("This .ato file is currently not compatible");
            msgBox.exec();
            return;
        }
    }

    //check if component files are .mol files and rename in the molFileList accordingly
    for (int i = 0; i < molFileList.count(); i++)
    {
        QFile checkFile(workingDir_+molFileList.at(i)+".mol");
        if (checkFile.exists())
        {
            molFileList.replace(i, molFileList.at(i)+".mol");
        }
        else
        molFileList.replace(i, molFileList.at(i)+".ato");
    }

    //check component files exist in folder
    if (molFileList.count() != 0)
    {
        for (int i = 0; i < molFileList.count(); i++)
        {
            QFile file(workingDir_+molFileList.at(i));
            if (!file.exists())
            {
                QMessageBox msgBox;
                msgBox.setText("Could not find component file "+molFileList.at(i));
                msgBox.exec();
                return;
            }
        }
    }

    //read epsr .inp file to get data and weights files
    if (!epsrInpFileName_.isEmpty())
    {
        QFile fileinp(workingDir_+epsrInpFileName_);
        if(!fileinp.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox msgBox;
            msgBox.setText("Could not open EPSR.inp file");
            msgBox.exec();
            return;
        }

        QTextStream stream(&fileinp);
        QString line;
        QStringList dataLine;
        dataLine.clear();

        do {
            line = stream.readLine();
            dataLine = line.split(" ", QString::SkipEmptyParts);
            if (dataLine.count() != 0)
            {
                if (dataLine.at(0) == "datafile")
                {
                    dataFileList.append(dataLine.at(1));
                }
                if (dataLine.at(0) == "wtsfile")
                {
                    wtsFileList.append(dataLine.at(1));
                }
                if (dataLine.at(0) == "nrtype")
                {
                    nrtypeList.append(dataLine.at(1));
                }
            }
        } while (!line.isNull());
        fileinp.close();
    }

    //check number of weights files and data files are the same
    if (dataFileList.count() != wtsFileList.count())
    {
        QMessageBox msgBox;
        msgBox.setText("The number of data files and weights files is different. Import the simulation as just the simulation box and make the weights files for each dataset.");
        msgBox.exec();
        return;
    }

    //check data and wts files exist in folder
    if (dataFileList.count() != 0)
    {
        for (int i = 0; i < dataFileList.count(); i++)
        {
            QFile file(workingDir_+dataFileList.at(i));
            if (!file.exists())
            {
                QMessageBox msgBox;
                msgBox.setText("Could not find data file "+dataFileList.at(i));
                msgBox.exec();
                return;
            }
        }
    }
    if (wtsFileList.count() != 0)
    {
        for (int i = 0; i < wtsFileList.count(); i++)
        {
            QFile file(workingDir_+wtsFileList.at(i));
            if (!file.exists())
            {
                QMessageBox msgBox;
                msgBox.setText("Could not find weights file "+wtsFileList.at(i));
                msgBox.exec();
                return;
            }
        }
    }

    //write to .EPSR.pro file
    QString saveFileName = workingDir_+projectName_+".EPSR.pro";
    QFile file(saveFileName);

    if(!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox msgBox;
        msgBox.setText("Could not save to "+projectName_+".EPSR.pro");
        msgBox.exec();
        return;
    }

    QTextStream streamWrite(&file);

    //mol files and number of them in box
    if (molFileList.count() != 0)
    {
        for (int i = 0; i < molFileList.count(); i++)
        {
            streamWrite << "mol " << molFileList.at(i) << " " << "1" << "\n";
        }
    }

    //box ato file details
    if (!atoFileName_.isEmpty())
    {
        streamWrite << "boxAtoFileName " << atoFileName_ << "\n";
    }

    //data and wts files
    if (dataFileList.count() != 0)
    {
        for (int i = 0; i < dataFileList.count(); i++)
        {
            streamWrite << "data " << nrtypeList.at(i) << " " << dataFileList.at(i) << " " << "0" << "\n"; //normalisation assumed to be not normalised to totals************************************************************
        }
        for (int i = 0; i < wtsFileList.count(); i++)
        {
            streamWrite << "wts " << wtsFileList.at(i) << "\n";
        }
    }

    //EPSR.inp and .pcof files
    if (!epsrInpFileName_.isEmpty())
    {
        QString inpBaseFileName = epsrInpFileName_.split(".",QString::SkipEmptyParts).at(0);
        streamWrite << "EPSRinp " << inpBaseFileName << "\n";
    }

    file.resize(0);
    file.close();

    accept();
}

QString ImportDialog::getWorkingDir()
{
    return workingDir_;
}

QString ImportDialog::getProjectName()
{
    return projectName_;
}

QString ImportDialog::getScriptFile()
{
    return scriptFile_;
}
